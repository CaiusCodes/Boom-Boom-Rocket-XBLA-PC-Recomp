[CmdletBinding()]
param([string]$SdkDirectory,[string]$BuildDirectory,[string]$InstallDirectory,[int]$Jobs=6)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(-not $SdkDirectory){$SdkDirectory=Join-Path $root 'out/dependencies/rexglue-sdk'}
if(-not $BuildDirectory){$BuildDirectory=Join-Path $root 'out/sdk-build'}
if(-not $InstallDirectory){$InstallDirectory=Join-Path $root 'out/sdk-install'}
$SdkDirectory=[IO.Path]::GetFullPath($SdkDirectory)
$BuildDirectory=[IO.Path]::GetFullPath($BuildDirectory)
$InstallDirectory=[IO.Path]::GetFullPath($InstallDirectory)
& (Join-Path $PSScriptRoot 'Apply-SourcePatches.ps1') -Directory $SdkDirectory -Manifest (Join-Path $root 'patches/sdk.json')
$sourceMap=$SdkDirectory.Replace('\','/')
$buildMap=$BuildDirectory.Replace('\','/')
$flags="-march=x86-64-v3 -ffile-prefix-map=$sourceMap/=rexglue-sdk/ -ffile-prefix-map=$buildMap/=sdk-build/"
& cmake -S $SdkDirectory -B $BuildDirectory -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ "-DCMAKE_C_FLAGS=$flags" "-DCMAKE_CXX_FLAGS=$flags" "-DCMAKE_INSTALL_PREFIX=$InstallDirectory" -DREXGLUE_USE_D3D12=ON -DREXGLUE_USE_VULKAN=OFF -DREXGLUE_BUILD_TESTS=OFF
if($LASTEXITCODE -ne 0){throw 'SDK configure failed'}
& cmake --build $BuildDirectory --parallel $Jobs
if($LASTEXITCODE -ne 0){throw 'SDK build failed'}
& cmake --install $BuildDirectory
if($LASTEXITCODE -ne 0){throw 'SDK install failed'}
Write-Host "SDK installed: $InstallDirectory"
