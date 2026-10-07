[CmdletBinding()]
param([string]$BuildPreset='win-amd64-release',[string]$RuntimeDirectory,[string]$ReleaseVersion='0.9.1')
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
if ($BuildPreset -notmatch '^[A-Za-z0-9_-]+$') { throw 'Invalid build preset.' }
if ($ReleaseVersion -notmatch '^[A-Za-z0-9._-]+$') { throw 'Invalid release version.' }
$projectRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot=Join-Path $projectRoot "out\build\$BuildPreset"
if ($RuntimeDirectory) { $buildRoot=[IO.Path]::GetFullPath($RuntimeDirectory) }
$setupSource=Join-Path $projectRoot 'setup'
$compiler=Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $compiler)) { throw '.NET Framework C# compiler not found.' }
$runtime=@('boom_boom_rocket.exe','rexruntime.dll','rexgpu-xenos.dll','TracyClient.dll')
foreach ($name in $runtime) { if (-not (Test-Path -LiteralPath (Join-Path $buildRoot $name))) { throw "Build the game first: missing $name" } }

# Check the matching host/plugin ABI before embedding any runtime bytes.
if (-not ('BbrSetupGpuAbi' -as [type])) {
Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
public static class BbrSetupGpuAbi {
 [UnmanagedFunctionPointer(CallingConvention.Cdecl)] delegate uint Version();
 [DllImport("kernel32",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr LoadLibraryExW(string path,IntPtr file,uint flags);
 [DllImport("kernel32",CharSet=CharSet.Ansi)] static extern IntPtr GetProcAddress(IntPtr h,string name);
 [DllImport("kernel32")] static extern bool FreeLibrary(IntPtr h);
 public static uint Read(string path) {
  var h=LoadLibraryExW(path,IntPtr.Zero,0x1100);if(h==IntPtr.Zero)throw new Win32Exception(Marshal.GetLastWin32Error());
  try{var p=GetProcAddress(h,"rex_gpu_abi_version");if(p==IntPtr.Zero)throw new Exception("Missing ABI export");return ((Version)Marshal.GetDelegateForFunctionPointer(p,typeof(Version)))();}finally{FreeLibrary(h);}
 }
}
'@
}
if ([BbrSetupGpuAbi]::Read((Join-Path $buildRoot 'rexgpu-xenos.dll')) -ne 2) { throw 'Xenos plug-in ABI must be 2.' }

# Every build has an isolated staging directory. Prior releases are preserved.
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$work=Join-Path $projectRoot "out\setup-build\$stamp"
$payload=Join-Path $work 'payload'
$release=Join-Path $projectRoot "out\release\Setup-$ReleaseVersion-$stamp"
$gamePayload=Join-Path $payload 'Game'
$resources=Join-Path $gamePayload 'resources'
New-Item -ItemType Directory -Path $resources,$release | Out-Null
foreach ($name in $runtime) { Copy-Item -LiteralPath (Join-Path $buildRoot $name) -Destination $resources }
Copy-Item -LiteralPath (Join-Path $projectRoot 'packaging\boom_boom_rocket.toml') -Destination $resources
Copy-Item -LiteralPath (Join-Path $setupSource 'README.txt') -Destination $payload
Copy-Item -LiteralPath (Join-Path $projectRoot 'packaging\licenses') -Destination $payload -Recurse
# This interpreted helper is a runtime dependency for the existing internal
# import modes, not a development project. Keep it with the runtime resources.
New-Item -ItemType Directory -Path (Join-Path $resources 'tools') | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'tools\Extract-STFS.ps1') -Destination (Join-Path $resources 'tools')
$icon=Join-Path $work 'firework.ico'
& (Join-Path $PSScriptRoot 'New-FireworkIcon.ps1') -OutputPath $icon
$gameIcon=Join-Path $work 'game.ico'
& (Join-Path $PSScriptRoot 'New-FireworkIcon.ps1') -OutputPath $gameIcon -Design Game
$launcher=Join-Path $gamePayload 'Boom Boom Rocket.exe'
& $compiler /nologo /target:winexe /platform:x64 /optimize+ /debug- "/out:$launcher" "/win32icon:$gameIcon" "/win32manifest:$setupSource\app.manifest" /reference:System.dll /reference:System.Windows.Forms.dll (Join-Path $setupSource 'launcher\Program.cs')
if($LASTEXITCODE -ne 0){throw 'Play launcher compilation failed.'}

# Portable metadata contains only Game-relative paths and immutable runtime
# hashes. Imported retail content and mutable settings/saves are not included.
$manifestFiles=@('Boom Boom Rocket.exe')+@($runtime | ForEach-Object { 'resources/'+$_ })+@('resources/tools/Extract-STFS.ps1')
$supportedXex=[regex]::Match((Get-Content -LiteralPath (Join-Path $setupSource 'InstallEngine.cs') -Raw),'SupportedXexSha256="([A-F0-9]{64})"').Groups[1].Value
if($supportedXex.Length -ne 64){throw 'Supported executable revision fingerprint is missing.'}
$manifest=[ordered]@{
 schemaVersion=1; product='Boom Boom Rocket XBLA Recomp'; version='0.9.1'; buildTag=$ReleaseVersion
 xboxTitleId='5841086A'; pathsRelativeTo='manifestDirectory'; entryPoint='Boom Boom Rocket.exe'
 resources='resources'; gameAssets='resources/assets'; configuration='resources/boom_boom_rocket.toml'
 userData='resources/userdata'; requiresOriginalGamePackage=$true; bundlesOriginalGameContent=$false
 supportedOriginalXexSha256=$supportedXex
 files=@($manifestFiles | ForEach-Object {
  $file=Join-Path $gamePayload $_
  [ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash;size=(Get-Item -LiteralPath $file).Length}
 })
}
[IO.File]::WriteAllText((Join-Path $gamePayload 'release-manifest.json'),($manifest | ConvertTo-Json -Depth 5)+"`r`n",[Text.UTF8Encoding]::new($false))

# Generate labels from the actual game source to prevent order/ID drift.
$appPath=Join-Path $projectRoot 'src\boom_boom_rocket_app.h'
if (Test-Path -LiteralPath $appPath) {
 $app=Get-Content -LiteralPath $appPath -Raw
 $block=[regex]::Match($app,'const std::wstring pc_strings\s*=([\s\S]*?);').Groups[1].Value
 $labels=([regex]::Matches($block,'L"([^"]*)"') | ForEach-Object { $_.Groups[1].Value.Replace('\r',"`r").Replace('\n',"`n") }) -join ''
} else {
 $labels=(Get-Content -LiteralPath (Join-Path $setupSource 'PcStrings.txt') -Raw).TrimEnd("`r","`n").Replace("`r`n","`n").Replace("`n","`r`n")
}
if (-not $labels.StartsWith('IDS_PC_DISPLAY_MODE =') -or $labels.TrimEnd("`r","`n").Split("`n").Count -ne 79) { throw 'PC string extraction failed.' }
$labelsFile=Join-Path $work 'pc-strings.txt'
[IO.File]::WriteAllText($labelsFile,$labels,[Text.UTF8Encoding]::new($false))

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=Join-Path $work 'payload.zip'
[IO.Compression.ZipFile]::CreateFromDirectory($payload,$zip,[IO.Compression.CompressionLevel]::Optimal,$false)
$entries=[IO.Compression.ZipFile]::OpenRead($zip)
try {
 $expected=@('Game/Boom Boom Rocket.exe','Game/release-manifest.json','README.txt')+@($runtime | ForEach-Object { 'Game/resources/'+$_ })+@('Game/resources/boom_boom_rocket.toml','Game/resources/tools/Extract-STFS.ps1')+@(Get-ChildItem -LiteralPath (Join-Path $payload 'licenses') -File | ForEach-Object { 'licenses/'+$_.Name })
 foreach($entry in $entries.Entries){if($entry.FullName.Replace('\','/') -notin $expected){throw "Unexpected setup payload: $($entry.FullName)"}}
 if($entries.Entries.Count -ne $expected.Count){throw 'Payload file count mismatch.'}
} finally { $entries.Dispose() }

$exe=Join-Path $release 'Setup Boom Boom Rocket.exe'
$sources=@(Get-ChildItem -LiteralPath $setupSource -Filter '*.cs' -File | Select-Object -ExpandProperty FullName)
& $compiler /nologo /target:winexe /platform:x64 /optimize+ /debug- "/out:$exe" "/win32icon:$icon" "/win32manifest:$setupSource\app.manifest" /reference:System.dll /reference:System.Core.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll /reference:System.IO.Compression.dll /reference:System.IO.Compression.FileSystem.dll "/resource:$zip,Bbr.Payload" "/resource:$labelsFile,Bbr.PcStrings" "/resource:$setupSource\PlayerNames.txt,Bbr.PlayerNames" @sources
if($LASTEXITCODE -ne 0){throw 'Setup compilation failed.'}
$hash=Get-FileHash -LiteralPath $exe -Algorithm SHA256
Write-Output "Setup EXE: $exe"
Write-Output ('Size: {0:N2} MB' -f ((Get-Item -LiteralPath $exe).Length/1MB))
Write-Output "SHA256: $($hash.Hash)"
