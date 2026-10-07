[CmdletBinding()]
param([Parameter(Mandatory)][string]$SetupExe,[string]$ReleaseVersion='0.9.1',[string]$ReleaseDirectory)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
if ($ReleaseVersion -notmatch '^[A-Za-z0-9._-]+$') { throw 'Invalid release version.' }
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if(-not(Test-Path -LiteralPath $SetupExe -PathType Leaf)){throw 'Build the setup EXE first.'}
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$work=Join-Path $root "out\portable-package\$stamp"
$portable=Join-Path $work 'Boom Boom Rocket XBLA PC Recomp'
$source=Join-Path $work 'Boom Boom Rocket XBLA PC Recomp Source'
$release=Join-Path $root 'out\release'
if($ReleaseDirectory){$release=[IO.Path]::GetFullPath($ReleaseDirectory)}
New-Item -ItemType Directory -Path $release -Force | Out-Null
$portableZip=Join-Path $release "Boom-Boom-Rocket-XBLA-PC-Recomp-v$ReleaseVersion.zip"
$sourceZip=Join-Path $release "Boom-Boom-Rocket-XBLA-PC-Recomp-v$ReleaseVersion-Source.zip"
foreach($file in @($portableZip,$sourceZip)){if(Test-Path -LiteralPath $file){throw "Existing release preserved: $file"}}
New-Item -ItemType Directory -Path $portable,$source | Out-Null
Copy-Item -LiteralPath $SetupExe -Destination (Join-Path $portable 'Setup Boom Boom Rocket.exe')
Copy-Item -LiteralPath (Join-Path $root 'setup\README.txt') -Destination $portable
Copy-Item -LiteralPath (Join-Path $root 'packaging\licenses') -Destination $portable -Recurse
$sourceFiles=@('CMakeLists.txt','CMakePresets.json','boom_boom_rocket_manifest.toml','generated\rexglue.cmake','LICENSE','README.md','VERSION','.gitignore','.gitattributes','Make Release.bat','Make Setup.bat')+@(
 'setup\SetupForm.cs','setup\Program.cs','setup\InstallEngine.cs','setup\StfsPackage.cs',
 'setup\PlayerNames.cs','setup\PlayerNames.txt',
 'setup\app.manifest','setup\PcStrings.txt','setup\README.txt','setup\tests\InstallerTests.cs',
 'setup\launcher\Program.cs','setup\tests\LaunchProbe.cs','tools\New-FireworkIcon.ps1',
 'setup\tests\SetupLayoutTests.cs','tools\Test-SetupLayout.ps1',
 'tools\Test-GameIcons.ps1',
 'tools\Build-Setup.ps1','tools\Test-Setup.ps1','tools\Extract-STFS.ps1',
 'packaging\boom_boom_rocket.toml'
)
# Canonical project-owned source only; backup suffixes cannot match these globs.
foreach($folder in @('src','tests','tools','patches')) {
 $sourceFiles+=@(Get-ChildItem -LiteralPath (Join-Path $root $folder) -File | Where-Object {$_.Extension -in @('.cpp','.h','.rc','.ps1','.json','.md')} | ForEach-Object {$_.FullName.Substring($root.Length+1)})
}
$sourceFiles=$sourceFiles | Sort-Object -Unique
foreach($relative in $sourceFiles){
 $dest=Join-Path $source $relative
 New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($dest)) -Force | Out-Null
 Copy-Item -LiteralPath (Join-Path $root $relative) -Destination $dest
}
Copy-Item -LiteralPath (Join-Path $root 'packaging\licenses') -Destination (Join-Path $source 'packaging') -Recurse
Copy-Item -LiteralPath (Join-Path $root 'packaging\github\RELEASE-NOTES.md') -Destination $source
Copy-Item -LiteralPath (Join-Path $root 'packaging\DEPENDENCY-SOURCE.txt') -Destination (Join-Path $source 'packaging')
if(Test-Path -LiteralPath (Join-Path $root 'docs')){Copy-Item -LiteralPath (Join-Path $root 'docs') -Destination $source -Recurse}
$ownLicense=Join-Path $root 'LICENSE'

Add-Type -AssemblyName System.IO.Compression.FileSystem
# The end-user archive has exactly the requested pre-setup tree. The separate
# source archive below is never mixed into it or installed by setup.
$publicFiles=@('Setup Boom Boom Rocket.exe','README.txt')+@(Get-ChildItem -LiteralPath (Join-Path $portable 'licenses') -File | ForEach-Object {'licenses/'+$_.Name})
$actualFiles=@(Get-ChildItem -LiteralPath $portable -File -Recurse | ForEach-Object {$_.FullName.Substring($portable.Length+1).Replace('\','/')})
if(@(Compare-Object $publicFiles $actualFiles).Count){throw 'Unexpected file in the end-user pre-setup tree.'}
[IO.Compression.ZipFile]::CreateFromDirectory($portable,$portableZip,[IO.Compression.CompressionLevel]::Optimal,$true)
[IO.Compression.ZipFile]::CreateFromDirectory($source,$sourceZip,[IO.Compression.CompressionLevel]::Optimal,$true)
foreach($zip in @($portableZip,$sourceZip)){
 $archive=[IO.Compression.ZipFile]::OpenRead($zip)
 try{
  foreach($entry in $archive.Entries){
   if($entry.FullName -match '(^|/)(assets|userdata|private|logs|assets_import_tmp)/|generated/default/|\.xex$|\.xma$|\.xpr$|\.bin$|\.bak$'){throw "Private file entered archive: $($entry.FullName)"}
  }
  Write-Output "$([IO.Path]::GetFileName($zip)): $($archive.Entries.Count) entries"
 }finally{$archive.Dispose()}
 Get-FileHash -LiteralPath $zip -Algorithm SHA256 | Format-List Path,Hash
}
Write-Output "Installer source folder: $source"
Write-Output "Licensing selected: $(Test-Path -LiteralPath $ownLicense)"
