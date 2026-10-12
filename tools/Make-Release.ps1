[CmdletBinding()]
param(
    [string]$BuildPreset = 'win-amd64-release',
    [string]$Version = '0.9.1'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Keep the historical entry point, but use the single current packaging path.
# Build-Launcher creates isolated staging with ordinary runtime files;
# Package-Launcher preserves old releases and enforces the public allowlist.
$buildOutput = @(& (Join-Path $PSScriptRoot 'Build-Launcher.ps1') -BuildPreset $BuildPreset -ReleaseVersion $Version)
$buildOutput | Write-Output
$setupLines = @($buildOutput | Where-Object { $_ -is [string] -and $_.StartsWith('Portable folder: ') })
if ($setupLines.Count -ne 1) { throw 'The launcher build did not return exactly one portable folder.' }
$portableFolder = $setupLines[0].Substring('Portable folder: '.Length)
$packageOutput=@(& (Join-Path $PSScriptRoot 'Package-Launcher.ps1') -PortableFolder $portableFolder -Version $Version)
$packageOutput|Write-Output
$zipLines=@($packageOutput|Where-Object{$_ -is [string] -and $_.StartsWith('Portable ZIP: ')})
if($zipLines.Count -ne 1){throw 'Packaging did not return exactly one archive.'}
# LGPL runtime libraries need matching sources alongside binary downloads.
$portableZip=$zipLines[0].Substring('Portable ZIP: '.Length)
$sourceZip=Join-Path (Split-Path $portableZip -Parent) "Boom-Boom-Rocket-XBLA-PC-Recomp-v$Version-Dependency-Source.zip"
& (Join-Path $PSScriptRoot 'Package-DependencySource.ps1') -Destination $sourceZip
