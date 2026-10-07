[CmdletBinding()]
param([Parameter(Mandatory)][string]$Directory,[Parameter(Mandatory)][string]$Manifest)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$utf8=[Text.UTF8Encoding]::new($false)
function Digest([string]$text) {
 $sha=[Security.Cryptography.SHA256]::Create()
 try{([BitConverter]::ToString($sha.ComputeHash($utf8.GetBytes($text)))).Replace('-','')}finally{$sha.Dispose()}
}
$root=[IO.Path]::GetFullPath($Directory)
$spec=Get-Content -LiteralPath $Manifest -Raw | ConvertFrom-Json
if($spec.schemaVersion -ne 1){throw 'Unsupported patch manifest'}
$pending=@()
foreach($file in $spec.files) {
 if($file.file -match '(^/|^[A-Za-z]:|\.\.)'){throw 'Invalid patch filename'}
 $path=[IO.Path]::GetFullPath((Join-Path $root $file.file))
 if(-not $path.StartsWith($root+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Patch escaped root'}
 $text=[IO.File]::ReadAllText($path).Replace("`r`n","`n")
 $hash=Digest $text
 if($hash -eq $file.patchedSha256){continue}
 if($hash -ne $file.rawSha256){throw "Unexpected source revision: $($file.file). No files were changed."}
 $lines=[Collections.Generic.List[string]]::new()
 $lines.AddRange([string[]]$text.Split([char]10))
 foreach($edit in ($file.edits | Sort-Object index -Descending)) {
  if($edit.index -lt 0 -or $edit.remove -lt 0 -or $edit.index+$edit.remove -gt $lines.Count){throw 'Invalid patch range'}
  $lines.RemoveRange($edit.index,$edit.remove)
  $lines.InsertRange($edit.index,[string[]]$edit.insert)
 }
 $result=$lines -join "`n"
 if((Digest $result) -ne $file.patchedSha256){throw "Patched hash mismatch: $($file.file). No files were changed."}
 $pending+=@{path=$path;text=$result}
}
# Validate every file before writing any, so mismatched regeneration cannot
# silently replace only part of the working title/dependency.
foreach($file in $pending){[IO.File]::WriteAllText($file.path,$file.text,$utf8)}
Write-Host "Verified $($spec.files.Count) files; applied $($pending.Count) deterministic patches."
