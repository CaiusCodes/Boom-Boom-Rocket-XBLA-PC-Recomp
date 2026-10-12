[CmdletBinding()]
param([string]$BuildPreset='win-amd64-release',[string]$RuntimeDirectory,[string]$ReleaseVersion='0.9.1')
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
if($BuildPreset -notmatch '^[A-Za-z0-9_-]+$' -or $ReleaseVersion -notmatch '^[A-Za-z0-9._-]+$'){throw 'Invalid build/version.'}
$root=Split-Path $PSScriptRoot -Parent
$runtime=Join-Path $root "out/build/$BuildPreset"
if($RuntimeDirectory){$runtime=[IO.Path]::GetFullPath($RuntimeDirectory)}
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$release=Join-Path $root "out/launcher-build/$stamp/Boom Boom Rocket"
$resources=Join-Path $release 'resources'
New-Item -ItemType Directory -Path $resources | Out-Null
$runtimeFiles=@('boom_boom_rocket.exe','rexruntime.dll','rexgpu-xenos.dll','TracyClient.dll')
if(-not ('BbrLauncherGpuAbi' -as [type])){
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class BbrLauncherGpuAbi {
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate uint Version();
 [DllImport("kernel32",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr LoadLibraryExW(string p,IntPtr f,uint flags);
 [DllImport("kernel32",CharSet=CharSet.Ansi)] static extern IntPtr GetProcAddress(IntPtr h,string p);
 [DllImport("kernel32")] static extern bool FreeLibrary(IntPtr h);
 public static uint Read(string path){var h=LoadLibraryExW(path,IntPtr.Zero,0x1100);if(h==IntPtr.Zero)throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());try{var p=GetProcAddress(h,"rex_gpu_abi_version");if(p==IntPtr.Zero)throw new Exception("Missing GPU ABI export");return ((Version)Marshal.GetDelegateForFunctionPointer(p,typeof(Version)))();}finally{FreeLibrary(h);}}
}
'@
}
if([BbrLauncherGpuAbi]::Read((Join-Path $runtime 'rexgpu-xenos.dll')) -ne 2){throw 'The runtime requires an ABI 2 GPU plug-in.'}
foreach($file in $runtimeFiles){Copy-Item -LiteralPath (Join-Path $runtime $file) -Destination $resources}
Copy-Item -LiteralPath (Join-Path $root 'packaging/boom_boom_rocket.toml') -Destination (Join-Path $resources 'defaults.toml')
Copy-Item -LiteralPath (Join-Path $root 'setup/README.txt') -Destination $release
Copy-Item -LiteralPath (Join-Path $root 'packaging/licenses') -Destination $release -Recurse
$icon=Join-Path (Split-Path $release -Parent) 'game.ico'
& (Join-Path $PSScriptRoot 'New-FireworkIcon.ps1') -OutputPath $icon -Design Game
$app=Get-Content -LiteralPath (Join-Path $root 'src/boom_boom_rocket_app.h') -Raw
$block=[regex]::Match($app,'const std::wstring pc_strings\s*=([\s\S]*?);').Groups[1].Value
$labels=([regex]::Matches($block,'L"([^"]*)"')|ForEach-Object{$_.Groups[1].Value.Replace('\r',"`r").Replace('\n',"`n")}) -join ''
if($labels.TrimEnd("`r","`n").Split("`n").Count -ne 81){throw 'Unexpected PC string table.'}
$strings=Join-Path (Split-Path $release -Parent) 'pc-strings.txt'
[IO.File]::WriteAllText($strings,$labels,[Text.UTF8Encoding]::new($false))
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$exe=Join-Path $release 'Boom Boom Rocket.exe'
$sources=@('setup/launcher/Program.cs','setup/launcher/ImportForm.cs','setup/launcher/LauncherTheme.cs','setup/launcher/PortableGame.cs','setup/StfsPackage.cs','setup/InstallEngine.cs','setup/PlayerNames.cs')|ForEach-Object{Join-Path $root $_}
& $compiler /nologo /target:winexe /platform:x64 /optimize+ /debug- /define:BBR_PORTABLE_LAUNCHER "/out:$exe" "/win32icon:$icon" "/win32manifest:$root/setup/app.manifest" /reference:System.dll /reference:System.Core.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll /reference:System.IO.Compression.dll /reference:System.Web.Extensions.dll "/resource:$strings,Bbr.PcStrings" "/resource:$root/setup/PlayerNames.txt,Bbr.PlayerNames" @sources
if($LASTEXITCODE -ne 0){throw 'Launcher compilation failed.'}
$manifest=[ordered]@{schemaVersion=2;product='Boom Boom Rocket XBLA PC Recomp';version=$ReleaseVersion;xboxTitleId='5841086A';pathsRelativeTo='launcherDirectory';entryPoint='Boom Boom Rocket.exe';resources='resources';gameAssets='resources/assets';configuration='resources/boom_boom_rocket.toml';userData='resources/userdata';bundlesOriginalGameContent=$false;files=@()}
foreach($file in @('Boom Boom Rocket.exe')+@($runtimeFiles|ForEach-Object{'resources/'+$_})+@('resources/defaults.toml')){
 $path=Join-Path $release $file
 $manifest.files+=@{path=$file;sha256=(Get-FileHash -LiteralPath $path).Hash;size=(Get-Item -LiteralPath $path).Length}
}
[IO.File]::WriteAllText((Join-Path $resources 'release-manifest.json'),($manifest|ConvertTo-Json -Depth 6),[Text.UTF8Encoding]::new($false))
$files=Get-ChildItem -LiteralPath $release -File -Recurse
$checksums=foreach($file in $files){'{0}  {1}' -f (Get-FileHash -LiteralPath $file.FullName).Hash,$file.FullName.Substring($release.Length+1).Replace('\','/')}
[IO.File]::WriteAllLines((Join-Path $resources 'SHA256SUMS.txt'),$checksums,[Text.UTF8Encoding]::new($false))
Write-Output "Launcher EXE: $exe"
Write-Output "Portable folder: $release"
Write-Output "SHA256: $((Get-FileHash -LiteralPath $exe).Hash)"
