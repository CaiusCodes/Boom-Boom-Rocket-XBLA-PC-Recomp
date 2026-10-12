# Historical build entry point. Produces the single-launcher portable folder,
# not the retired self-extracting setup executable.
param([string]$BuildPreset='win-amd64-release',[string]$RuntimeDirectory,[string]$ReleaseVersion='0.9.1')
& (Join-Path $PSScriptRoot 'Build-Launcher.ps1') @PSBoundParameters
