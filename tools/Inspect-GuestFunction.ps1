param([Parameter(Mandatory)][string[]]$Name)
# Read-only static inspection of translated guest functions; no runtime tracing.
$ErrorActionPreference='Stop'
$source=Join-Path $PSScriptRoot '..\generated\default'
foreach($function in $Name){
 if($function -notmatch '^sub_[0-9A-Fa-f]{8}$'){throw 'Expected a translated function name.'}
 $file=& rg -l -F "DEFINE_REX_FUNC($function)" $source -g '*recomp.*.cpp'
 if(!$file){throw "Function not found: $function"}
 $text=[IO.File]::ReadAllText($file)
 $start=$text.IndexOf("DEFINE_REX_FUNC($function)",[StringComparison]::Ordinal)
 $end=$text.IndexOf('DEFINE_REX_FUNC(',$start+20,[StringComparison]::Ordinal)
 if($end -lt 0){$end=$text.Length}
 $text.Substring($start,$end-$start).Split("`n")
}
