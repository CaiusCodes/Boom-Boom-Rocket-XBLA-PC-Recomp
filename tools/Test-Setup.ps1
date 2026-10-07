param([Parameter(Mandatory)][string]$GamePackage,[Parameter(Mandatory)][string]$DlcPackage)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$latest=Get-ChildItem -LiteralPath (Join-Path $root 'out\setup-build') -Directory | Sort-Object Name -Descending | Select-Object -First 1
if(-not $latest){throw 'Build setup first.'}
$work=Join-Path $root ('out\setup-tests\checks-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $work | Out-Null
$exe=Join-Path $work 'InstallerTests.exe'
$compiler=Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$probe=Join-Path $work 'LaunchProbe.exe'
& $compiler /nologo /target:winexe /platform:x64 "/out:$probe" (Join-Path $root 'setup\tests\LaunchProbe.cs')
if($LASTEXITCODE -ne 0){throw 'Launcher probe compilation failed.'}
& $compiler /nologo /target:exe /platform:x64 /optimize+ "/out:$exe" /reference:System.dll /reference:System.Core.dll /reference:System.IO.Compression.dll /reference:System.Web.Extensions.dll "/resource:$($latest.FullName)\payload.zip,Bbr.Payload" "/resource:$($latest.FullName)\pc-strings.txt,Bbr.PcStrings" "/resource:$root\setup\PlayerNames.txt,Bbr.PlayerNames" (Join-Path $root 'setup\PlayerNames.cs') (Join-Path $root 'setup\StfsPackage.cs') (Join-Path $root 'setup\InstallEngine.cs') (Join-Path $root 'setup\tests\InstallerTests.cs')
if($LASTEXITCODE -ne 0){throw 'Test compilation failed.'}
& $exe $GamePackage $DlcPackage (Join-Path $work 'fixtures') $probe
if($LASTEXITCODE -ne 0){throw 'Installer tests failed.'}
