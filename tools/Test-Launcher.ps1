param([Parameter(Mandatory)][string]$AssetsDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$work=Join-Path $root ('out/launcher-tests/'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $work | Out-Null
$exe=Join-Path $work 'PortableLauncherTests.exe'
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$probe=Join-Path $work 'LaunchProbe.exe'
& $compiler /nologo /target:winexe /platform:x64 "/out:$probe" (Join-Path $root 'setup/tests/LaunchProbe.cs')
if($LASTEXITCODE -ne 0){throw 'Launch probe compilation failed.'}
$sources=@('setup/tests/PortableLauncherTests.cs','setup/launcher/PortableGame.cs','setup/launcher/ImportForm.cs','setup/launcher/LauncherTheme.cs','setup/StfsPackage.cs','setup/InstallEngine.cs','setup/PlayerNames.cs')|ForEach-Object{Join-Path $root $_}
& $compiler /nologo /target:exe /platform:x64 /optimize+ '/define:BBR_PORTABLE_LAUNCHER,BBR_LAUNCHER_TESTS' "/out:$exe" /reference:System.dll /reference:System.Core.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll /reference:System.IO.Compression.dll /reference:System.Web.Extensions.dll "/resource:$root/setup/PcStrings.txt,Bbr.PcStrings" "/resource:$root/setup/PlayerNames.txt,Bbr.PlayerNames" @sources
if($LASTEXITCODE -ne 0){throw 'Test compilation failed.'}
& $exe (Join-Path $work 'fixtures') $AssetsDirectory $probe
if($LASTEXITCODE -ne 0){throw 'Portable launcher tests failed.'}
Write-Output "Test folder: $work"
