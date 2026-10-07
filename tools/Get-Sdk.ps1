[CmdletBinding()]
param([string]$Destination,[string]$LocalMirror,[switch]$SkipPatches)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(-not $Destination){$Destination=Join-Path $root 'out/dependencies/rexglue-sdk'}
$Destination=[IO.Path]::GetFullPath($Destination)
$pin='3eb9b511b4140d2769e27be63eae57d41bfa2afa'
function GitRun([string[]]$Arguments){ & git @Arguments;if($LASTEXITCODE -ne 0){throw "Git failed: $($Arguments[0])"} }
if(-not (Test-Path -LiteralPath (Join-Path $Destination '.git'))) {
 if(Test-Path -LiteralPath $Destination){throw 'Choose a new empty SDK destination; existing files are never replaced.'}
 $origin='https://github.com/rexglue/rexglue-sdk.git'
 $cloneSource=$origin;if($LocalMirror){$cloneSource=[IO.Path]::GetFullPath($LocalMirror)}
 GitRun @('clone','--no-hardlinks','--no-checkout',$cloneSource,$Destination)
 GitRun @('-C',$Destination,'remote','set-url','origin',$origin)
 GitRun @('-C',$Destination,'checkout','--detach',$pin)
}
$head=& git -C $Destination rev-parse HEAD
if($head -ne $pin){throw 'SDK checkout does not match the pinned commit.'}
# Only the dependencies used by the Windows D3D12/tool build are fetched.
$modules=@('cli11','libmspack','FFmpeg','tomlplusplus','simde','xxHash','spdlog','fmt','snappy','utfcpp','imgui','sdl3','tracy','o1heap','inja')
foreach($module in $modules) {
 $relative='thirdparty/'+$module
 $expected=(& git -C $Destination ls-tree HEAD -- $relative).Split()[2]
 if($LocalMirror -and -not (Test-Path -LiteralPath (Join-Path $Destination "$relative/.git"))) {
  GitRun @('clone','--no-hardlinks','--no-checkout',(Join-Path $LocalMirror $relative),(Join-Path $Destination $relative))
  GitRun @('-C',(Join-Path $Destination $relative),'checkout','--detach',$expected)
  $url= & git -C $Destination config -f .gitmodules --get "submodule.$relative.url"
  GitRun @('-C',(Join-Path $Destination $relative),'remote','set-url','origin',$url)
 } elseif(-not (Test-Path -LiteralPath (Join-Path $Destination "$relative/.git"))) {
  GitRun @('-C',$Destination,'submodule','update','--init','--recursive','--',$relative)
 }
 $actual=& git -C (Join-Path $Destination $relative) rev-parse HEAD
 if($actual -ne $expected){throw "Dependency pin mismatch: $module"}
}
if(-not $SkipPatches){& (Join-Path $PSScriptRoot 'Apply-SourcePatches.ps1') -Directory $Destination -Manifest (Join-Path $root 'patches/sdk.json')}
Write-Host "Pinned SDK ready: $Destination"
