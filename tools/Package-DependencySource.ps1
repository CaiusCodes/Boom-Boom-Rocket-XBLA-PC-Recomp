[CmdletBinding()]
param([string]$SdkDirectory,[Parameter(Mandatory)][string]$Destination)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(-not $SdkDirectory){$SdkDirectory=Join-Path $root 'out/dependencies/rexglue-sdk'}
if(Test-Path -LiteralPath $Destination){throw 'Existing source archive preserved; choose a new destination.'}
$stage=Join-Path $root ('out/dependency-source/'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
$source=Join-Path $stage 'sdk-source'
New-Item -ItemType Directory -Path $source | Out-Null
$pin='3eb9b511b4140d2769e27be63eae57d41bfa2afa'
if((& git -C $SdkDirectory rev-parse HEAD) -ne $pin){throw 'Wrong SDK source revision'}
function ExportRepo([string]$repo,[string]$output) {
 $archive=Join-Path $stage ('archive-'+[Guid]::NewGuid().ToString('N')+'.zip')
 & git -C $repo archive --format=zip "--output=$archive" HEAD
 if($LASTEXITCODE -ne 0){throw 'Source export failed'}
 Expand-Archive -LiteralPath $archive -DestinationPath $output
}
# Export Git-tracked pinned source only: never working-directory backups, logs,
# installed SDKs, binaries, caches or another title's custom SDK additions.
ExportRepo $SdkDirectory $source
& (Join-Path $PSScriptRoot 'Apply-SourcePatches.ps1') -Directory $source -Manifest (Join-Path $root 'patches/sdk.json')
$pins=@()
foreach($module in @('cli11','libmspack','FFmpeg','tomlplusplus','simde','xxHash','spdlog','fmt','snappy','utfcpp','imgui','sdl3','tracy','o1heap','inja')) {
 $relative='thirdparty/'+$module;$repo=Join-Path $SdkDirectory $relative
 $expected=(& git -C $SdkDirectory ls-tree HEAD -- $relative).Split()[2]
 $actual=& git -C $repo rev-parse HEAD
 if($actual -ne $expected){throw "Unexpected dependency revision: $module"}
 if(@(& git -C $repo status --porcelain --untracked-files=no).Count){throw "Modified upstream dependency: $module"}
 ExportRepo $repo (Join-Path $source $relative)
 $pins+="$module $actual"
 # The upstream SDK source-tarball check requires a .git marker; no Git database
 # or developer remote/config is distributed. Source-kit CMake needs only existence.
 New-Item -ItemType Directory -Path (Join-Path $source "$relative/.git") | Out-Null
 [IO.File]::WriteAllText((Join-Path $source "$relative/.git/SOURCE-ARCHIVE"),'Pinned source export; not a Git repository.')
}
New-Item -ItemType Directory -Path (Join-Path $stage 'tools'),(Join-Path $stage 'patches') | Out-Null
foreach($name in @('Build-Sdk.ps1','Apply-SourcePatches.ps1')){Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination (Join-Path $stage 'tools')}
Copy-Item -LiteralPath (Join-Path $root 'patches/sdk.json') -Destination (Join-Path $stage 'patches')
Copy-Item -LiteralPath (Join-Path $root 'packaging/DEPENDENCY-SOURCE.txt') -Destination (Join-Path $stage 'README.txt')
Copy-Item -LiteralPath (Join-Path $root 'packaging/licenses') -Destination (Join-Path $stage 'licenses') -Recurse
# The developer-only GNU disassembler is not linked into shipped runtime/GPU
# DLLs, but its source is in this SDK source export and retains GPL notices.
Copy-Item -LiteralPath (Join-Path $SdkDirectory 'thirdparty/FFmpeg/COPYING.GPLv2') -Destination (Join-Path $stage 'licenses/LICENSE-GNU-disassembler.txt')
[IO.File]::WriteAllLines((Join-Path $stage 'DEPENDENCY-PINS.txt'),@("ReXGlue $pin")+$pins,[Text.UTF8Encoding]::new($false))
$files=@('sdk-source','tools','patches','licenses','README.txt','DEPENDENCY-PINS.txt')
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[IO.Compression.ZipFile]::Open($Destination,[IO.Compression.ZipArchiveMode]::Create)
try{
 foreach($entry in $files){
  $path=Join-Path $stage $entry
  $items=@();if(Test-Path -LiteralPath $path -PathType Container){$items=@(Get-ChildItem -LiteralPath $path -File -Recurse -Force)}else{$items=@(Get-Item -LiteralPath $path)}
  foreach($file in $items){$name=$file.FullName.Substring($stage.Length+1).Replace('\','/');[IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip,$file.FullName,$name,[IO.Compression.CompressionLevel]::Optimal)|Out-Null}
 }
}finally{$zip.Dispose()}
Write-Host "Matching SDK/dependency sources: $Destination"
