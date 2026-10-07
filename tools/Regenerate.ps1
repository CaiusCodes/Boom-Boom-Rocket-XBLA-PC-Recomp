[CmdletBinding(DefaultParameterSetName='Directory')]
param(
 [Parameter(Mandatory,ParameterSetName='Package')][string]$GamePackage,
 [Parameter(Mandatory,ParameterSetName='Directory')][string]$GameDirectory,
 [string]$SdkPrefix,[string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(-not $SdkPrefix){$SdkPrefix=Join-Path $root 'out/sdk-install'}
if(-not $OutputDirectory){$OutputDirectory=Join-Path $root 'generated/default'}
$generator=Join-Path $SdkPrefix 'bin/rexglue.exe'
if(-not (Test-Path -LiteralPath $generator)){throw 'Build the pinned SDK first (tools/Build-Sdk.ps1).'}
$stage=Join-Path $root ('out/codegen/'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $stage | Out-Null
if($GamePackage) {
 $GameDirectory=Join-Path $stage 'input'
 & (Join-Path $PSScriptRoot 'Extract-STFS.ps1') -Path $GamePackage -OutputDir $GameDirectory
}
$GameDirectory=[IO.Path]::GetFullPath($GameDirectory)
$xex=Join-Path $GameDirectory 'default.xex'
$spec=Get-Content -LiteralPath (Join-Path $root 'patches/generated.json') -Raw | ConvertFrom-Json
if((Get-FileHash -LiteralPath $xex -Algorithm SHA256).Hash -ne $spec.originalXexSha256){throw 'Unsupported Boom Boom Rocket executable revision.'}
$manifest=Get-Content -LiteralPath (Join-Path $root 'boom_boom_rocket_manifest.toml') -Raw
$portableInput=$GameDirectory.Replace('\','/').Replace('"','\"')
$manifest=[regex]::Replace($manifest,'(?m)^game_root = .*$',('game_root = "'+$portableInput+'"'))
$manifest=[regex]::Replace($manifest,'(?m)^file_path = .*$',('file_path = "'+$portableInput+'/default.xex"'))
$config=Join-Path $stage 'boom_boom_rocket_manifest.toml'
[IO.File]::WriteAllText($config,$manifest,[Text.UTF8Encoding]::new($false))
# CLI logging and every intermediate remain private, outside publishable source.
$p=Start-Process -FilePath $generator -ArgumentList @('--log-file',('"'+(Join-Path $stage 'codegen.log')+'"'),'codegen',('"'+$config+'"')) -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $stage 'stdout.txt') -RedirectStandardError (Join-Path $stage 'stderr.txt')
if($p.ExitCode -ne 0){throw "Code generation failed ($($p.ExitCode)); see $stage"}
$generated=Join-Path $stage 'generated/default'
& (Join-Path $PSScriptRoot 'Apply-SourcePatches.ps1') -Directory $generated -Manifest (Join-Path $root 'patches/generated.json')
$actual=@(Get-ChildItem -LiteralPath $generated -File | Select-Object -ExpandProperty Name)
if(@(Compare-Object $actual @($spec.files.file)).Count){throw 'Unexpected generator output set'}
# Copy only verified canonical output. Existing local backups are not touched.
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
foreach($file in $spec.files){Copy-Item -LiteralPath (Join-Path $generated $file.file) -Destination (Join-Path $OutputDirectory $file.file)}
Write-Host "Verified clean regeneration: $OutputDirectory"
