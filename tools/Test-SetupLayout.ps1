$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$work=Join-Path $root ('out\layout-tests\'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $work | Out-Null
$compiler=Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$exe=Join-Path $work 'SetupLayoutTests.exe'
& $compiler /nologo /target:exe /platform:x64 /optimize+ "/out:$exe" "/win32manifest:$root\setup\app.manifest" /reference:System.dll /reference:System.Core.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll /reference:System.IO.Compression.dll (Join-Path $root 'setup\PlayerNames.cs') (Join-Path $root 'setup\SetupForm.cs') (Join-Path $root 'setup\InstallEngine.cs') (Join-Path $root 'setup\StfsPackage.cs') (Join-Path $root 'setup\tests\SetupLayoutTests.cs')
if($LASTEXITCODE -ne 0){throw 'Layout test compilation failed.'}
& $exe
if($LASTEXITCODE -ne 0){throw 'Layout tests failed.'}
