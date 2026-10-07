param([Parameter(Mandatory)][string]$DumpPath,[string]$Name='Profile\Load.lua')
# Private diagnostic: reads a user-owned decompressed image, never writes its data.
$ErrorActionPreference='Stop'
$probe=[IO.File]::ReadAllBytes([IO.Path]::GetFullPath($DumpPath))
function ReadBE($o){[uint32]($probe[$o]*16777216+$probe[$o+1]*65536+$probe[$o+2]*256+$probe[$o+3])}
$found=$false
foreach($i in 0..45){
 $e=0x2f0008+16*$i;$n=0x2f02e8+(ReadBE $e)
 $entryName=[Text.Encoding]::ASCII.GetString($probe,$n,80).Split([char]0)[0]
 if($entryName -ne $Name){continue}
 $offset=0x2f02e8+(ReadBE ($e+4));$length=ReadBE ($e+8)
 $stream=[IO.MemoryStream]::new($probe,($offset+2),($length-2))
 $inflate=[IO.Compression.DeflateStream]::new($stream,[IO.Compression.CompressionMode]::Decompress)
 $out=[IO.MemoryStream]::new();$inflate.CopyTo($out);$inflate.Dispose();$out.Position=12
 $script:r=[IO.BinaryReader]::new($out);$found=$true;break
}
if(!$found){throw 'Script not found'}
function ReadString {$n=$script:r.ReadUInt32();if(!$n){return ''};[Text.Encoding]::UTF8.GetString($script:r.ReadBytes($n)).TrimEnd([char]0)}
$ops='MOVE LOADK LOADBOOL LOADNIL GETUPVAL GETGLOBAL GETTABLE SETGLOBAL SETUPVAL SETTABLE NEWTABLE SELF ADD SUB MUL DIV MOD POW UNM NOT LEN CONCAT JMP EQ LT LE TEST TESTSET CALL TAILCALL RETURN FORLOOP FORPREP TFORLOOP SETLIST CLOSE CLOSURE VARARG'.Split(' ')
function ReadFunction($label){
 $source=ReadString;$line=$script:r.ReadUInt32();$last=$script:r.ReadUInt32()
 $up=$script:r.ReadByte();$params=$script:r.ReadByte();$vararg=$script:r.ReadByte();$stack=$script:r.ReadByte()
 $codeOffset=$script:r.BaseStream.Position
 $count=$script:r.ReadUInt32();$code=@();for($j=0;$j -lt $count;$j++){$code+= $script:r.ReadUInt32()}
 $count=$script:r.ReadUInt32();$constants=@();for($j=0;$j -lt $count;$j++){
  $type=$script:r.ReadByte();$constants+=switch($type){0{'nil'}1{$script:r.ReadByte()}3{$script:r.ReadDouble()}4{ReadString}default{throw "Bad constant $type"}}
 }
 "FUNCTION $label stack=$stack code-offset=$codeOffset"
 for($j=0;$j -lt $constants.Count;$j++){" K$j = $($constants[$j])"}
 for($j=0;$j -lt $code.Count;$j++){
  $c=$code[$j];$op=$c-band 63;$a=($c-shr 6)-band 255;$b=($c-shr 23)-band 511;$cval=($c-shr 14)-band 511;$bx=$c-shr 14
  $detail=if($op -in 1,5,7){"K$bx ($($constants[$bx]))"}elseif($op -in 22,31,32){"target=$($j+1+$bx-131071)"}else{''}
  '{0,4} {1,-10} A={2} B={3} C={4} {5}' -f $j,$ops[$op],$a,$b,$cval,$detail
 }
 $count=$script:r.ReadUInt32();for($j=0;$j -lt $count;$j++){ReadFunction "$label.$j"}
 $count=$script:r.ReadUInt32();[void]$script:r.ReadBytes(4*$count)
 $count=$script:r.ReadUInt32();for($j=0;$j -lt $count;$j++){[void](ReadString);[void]$script:r.ReadUInt32();[void]$script:r.ReadUInt32()}
 $count=$script:r.ReadUInt32();for($j=0;$j -lt $count;$j++){[void](ReadString)}
}
try{ReadFunction 'root'}finally{$script:r.Dispose()}
