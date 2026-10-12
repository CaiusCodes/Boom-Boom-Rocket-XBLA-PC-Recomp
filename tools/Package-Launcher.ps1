[CmdletBinding()]
param([Parameter(Mandatory)][string]$PortableFolder,[string]$Version='0.9.1')
$ErrorActionPreference='Stop'
if($Version -notmatch '^[A-Za-z0-9._-]+$'){throw 'Invalid release version.'}
$root=Split-Path $PSScriptRoot -Parent
$PortableFolder=[IO.Path]::GetFullPath($PortableFolder)
$names=@(Get-ChildItem -LiteralPath $PortableFolder | ForEach-Object {$_.Name} | Sort-Object)
if(($names -join '|') -ne ((@('Boom Boom Rocket.exe','resources','README.txt','licenses')|Sort-Object)-join '|')){throw 'Unexpected portable root.'}
$expected=@('Boom Boom Rocket.exe','README.txt')+@(Get-ChildItem (Join-Path $PortableFolder 'licenses') -File|ForEach-Object{'licenses/'+$_.Name})+@('resources/boom_boom_rocket.exe','resources/rexruntime.dll','resources/rexgpu-xenos.dll','resources/TracyClient.dll','resources/defaults.toml','resources/release-manifest.json','resources/SHA256SUMS.txt')
$actual=@(Get-ChildItem -LiteralPath $PortableFolder -File -Recurse|ForEach-Object{$_.FullName.Substring($PortableFolder.Length+1).Replace('\','/')})
if(@(Compare-Object $expected $actual).Count){throw 'Unexpected file/content in portable release.'}
$release=Join-Path $root ('out/release/Launcher-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $release|Out-Null
$zip=Join-Path $release "Boom-Boom-Rocket-XBLA-PC-Recomp-v$Version.zip"
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($PortableFolder,$zip,[IO.Compression.CompressionLevel]::Optimal,$true)
[IO.File]::WriteAllText(($zip+'.sha256'),"$((Get-FileHash -LiteralPath $zip).Hash)  $([IO.Path]::GetFileName($zip))`n",[Text.UTF8Encoding]::new($false))
Write-Output "Portable ZIP: $zip"
