[CmdletBinding()]
param([Parameter(Mandatory)][string]$Directory,[string[]]$PersonalNames=@())
$ErrorActionPreference='Stop'
$patterns=@('(?i)[A-Z]:[\\/]+Users[\\/]+(?!\.\.\.)[^\x00\s"<>]+','(?i)[A-Z]:[\\/]+[^\x00\r\n]*Desktop[\\/]+[^\x00\r\n]*trace')
foreach($name in $PersonalNames){$patterns+='(?i)(?<![A-Za-z0-9_])'+[regex]::Escape($name)+'(?![A-Za-z0-9_])'}
$failures=@()
foreach($file in Get-ChildItem -LiteralPath $Directory -File -Recurse -Force) {
 $bytes=[IO.File]::ReadAllBytes($file.FullName)
 $texts=@([Text.Encoding]::UTF8.GetString($bytes),[Text.Encoding]::Unicode.GetString($bytes))
 foreach($pattern in $patterns){
  if(@($texts | Where-Object {[regex]::IsMatch($_,$pattern)}).Count){$failures+=$file.FullName;break}
 }
}
if($failures.Count){$failures|Write-Output;throw "Personal/local strings found in $($failures.Count) files"}
Write-Host "No personal/local path matches in $Directory. Compressed archives must be extracted before scanning."
