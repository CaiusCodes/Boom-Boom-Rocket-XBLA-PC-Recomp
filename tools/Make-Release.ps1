[CmdletBinding()]
param(
    [string]$BuildPreset = 'win-amd64-release',
    [string]$Version = '0.9.1'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Keep the historical entry point, but use the single current packaging path.
# Build-Setup validates the runtime/plugin and creates isolated staging;
# Package-Portable preserves old releases and enforces the public allowlist.
$buildOutput = @(& (Join-Path $PSScriptRoot 'Build-Setup.ps1') -BuildPreset $BuildPreset -ReleaseVersion $Version)
$buildOutput | Write-Output
$setupLines = @($buildOutput | Where-Object { $_ -is [string] -and $_.StartsWith('Setup EXE: ') })
if ($setupLines.Count -ne 1) { throw 'The setup build did not return exactly one executable path.' }
$setupExe = $setupLines[0].Substring('Setup EXE: '.Length)
& (Join-Path $PSScriptRoot 'Package-Portable.ps1') -SetupExe $setupExe -ReleaseVersion $Version
# LGPL runtime libraries need matching sources alongside binary downloads.
$sourceZip=Join-Path (Split-Path $PSScriptRoot -Parent) "out/release/Boom-Boom-Rocket-XBLA-PC-Recomp-v$Version-Dependency-Source.zip"
& (Join-Path $PSScriptRoot 'Package-DependencySource.ps1') -Destination $sourceZip
