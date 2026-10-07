param([Parameter(Mandatory)][string]$OutputPath,[ValidateSet('Setup','Game')][string]$Design='Setup')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
# Original code-drawn artwork; no retail game textures or logos are used.
$frames=@()
foreach($size in @(16,24,32,48,64,128,256)) {
 $bitmap=[Drawing.Bitmap]::new($size,$size)
 $g=[Drawing.Graphics]::FromImage($bitmap)
 try {
  $g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::AntiAlias
  $g.ScaleTransform([single]($size/256.0),[single]($size/256.0))
  $g.Clear([Drawing.Color]::Transparent)
  $tile=[Drawing.Drawing2D.GraphicsPath]::new()
  try {
   $tile.AddArc(6,6,52,52,180,90);$tile.AddArc(198,6,52,52,270,90)
   $tile.AddArc(198,198,52,52,0,90);$tile.AddArc(6,198,52,52,90,90);$tile.CloseFigure()
   $bg=[Drawing.Drawing2D.LinearGradientBrush]::new([Drawing.Point]::new(0,0),[Drawing.Point]::new(256,256),[Drawing.Color]::FromArgb(16,30,51),[Drawing.Color]::FromArgb(7,7,24))
   $edge=[Drawing.Pen]::new([Drawing.Color]::FromArgb(78,48,199,236),2)
   try{$g.FillPath($bg,$tile);$g.DrawPath($edge,$tile)}finally{$bg.Dispose();$edge.Dispose()}
  } finally {$tile.Dispose()}
  $cyan=[Drawing.Color]::FromArgb(38,224,255);$pink=[Drawing.Color]::FromArgb(255,55,208)
  if ($Design -eq 'Game') {
   # A diagonal neon rocket: intentionally different from setup's radial firework.
   $g.TranslateTransform(128,128);$g.RotateTransform(40);$g.TranslateTransform(-128,-128)
   $body=[Drawing.Drawing2D.GraphicsPath]::new()
   $body.AddBezier(103,159,92,105,104,57,128,30)
   $body.AddBezier(128,30,152,57,164,105,153,159)
   $body.CloseFigure()
   $fins=[Drawing.PointF[]]@([Drawing.PointF]::new(103,119),[Drawing.PointF]::new(79,163),[Drawing.PointF]::new(103,154),[Drawing.PointF]::new(153,154),[Drawing.PointF]::new(177,163),[Drawing.PointF]::new(153,119))
   $fill=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(12,48,71))
   try {
    $g.FillPath($fill,$body)
    foreach($pass in @(0,1,2)) {
     $alpha=@(25,65,255)[$pass];$width=@(20,11,4.5)[$pass]
     $outline=[Drawing.Pen]::new([Drawing.Color]::FromArgb($alpha,$cyan),[single]$width)
     $flame=[Drawing.Pen]::new([Drawing.Color]::FromArgb($alpha,$pink),[single]$width)
     $flame.StartCap=$flame.EndCap=[Drawing.Drawing2D.LineCap]::Round
     try {
      $g.DrawPolygon($flame,$fins);$g.DrawPath($outline,$body)
      $g.DrawEllipse($outline,115,89,26,26)
      $g.DrawLine($flame,115,174,115,207);$g.DrawLine($flame,128,174,128,232);$g.DrawLine($flame,141,174,141,207)
     } finally {$outline.Dispose();$flame.Dispose()}
    }
   } finally {$fill.Dispose();$body.Dispose()}
   $g.ResetTransform()
  } else {
  $count=if($size -le 24){12}else{20}
  for($i=0;$i -lt $count;$i++) {
   $a=$i*[Math]::PI*2/$count
   $radius=if($i%3 -eq 0){96}else{78+($i%4)*4}
   $color=if($i%2){$cyan}else{$pink}
   $x=[single](126+[Math]::Cos($a)*$radius);$y=[single](117+[Math]::Sin($a)*$radius)
   foreach($pass in @(0,1,2)) {
    $alpha=@(24,65,255)[$pass];$width=@(15,7,2.6)[$pass]
    $pen=[Drawing.Pen]::new([Drawing.Color]::FromArgb($alpha,$color),[single]$width)
    $pen.StartCap=[Drawing.Drawing2D.LineCap]::Round;$pen.EndCap=[Drawing.Drawing2D.LineCap]::Round
    try{$g.DrawBezier($pen,[single](126+[Math]::Cos($a)*17),[single](117+[Math]::Sin($a)*17),[single](126+[Math]::Cos($a+.16)*$radius*.5),[single](117+[Math]::Sin($a+.16)*$radius*.5),$x,[single]($y-8),$x,$y)}finally{$pen.Dispose()}
   }
   $dot=[Drawing.SolidBrush]::new($color)
   try{$g.FillEllipse($dot,[single]($x-2.5),[single]($y-2.5),5,5)}finally{$dot.Dispose()}
  }
  # A bright central star and two small satellite sparks retain clarity at 16px.
  foreach($spark in @(@(126,117,13),@(207,42,11),@(45,201,8))) {
   $x=$spark[0];$y=$spark[1];$r=$spark[2]
   $points=[Drawing.PointF[]]@([Drawing.PointF]::new($x,$y-$r),[Drawing.PointF]::new($x+3,$y-3),[Drawing.PointF]::new($x+$r,$y),[Drawing.PointF]::new($x+3,$y+3),[Drawing.PointF]::new($x,$y+$r),[Drawing.PointF]::new($x-3,$y+3),[Drawing.PointF]::new($x-$r,$y),[Drawing.PointF]::new($x-3,$y-3))
   $brush=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(228,252,255))
   try{$g.FillPolygon($brush,$points)}finally{$brush.Dispose()}
  }
  }
  $memory=[IO.MemoryStream]::new()
  try{$bitmap.Save($memory,[Drawing.Imaging.ImageFormat]::Png);$frames+=@{Size=$size;Bytes=$memory.ToArray()}}finally{$memory.Dispose()}
  if($size -eq 256){$bitmap.Save([IO.Path]::ChangeExtension($OutputPath,'.png'),[Drawing.Imaging.ImageFormat]::Png)}
 } finally {$g.Dispose();$bitmap.Dispose()}
}
$stream=[IO.File]::Create($OutputPath)
$writer=[IO.BinaryWriter]::new($stream)
try {
 $writer.Write([uint16]0);$writer.Write([uint16]1);$writer.Write([uint16]$frames.Count)
 $offset=6+16*$frames.Count
 foreach($frame in $frames) {
  $dimension=if($frame.Size -eq 256){0}else{$frame.Size}
  $writer.Write([byte]$dimension);$writer.Write([byte]$dimension);$writer.Write([byte]0);$writer.Write([byte]0)
  $writer.Write([uint16]1);$writer.Write([uint16]32);$writer.Write([uint32]$frame.Bytes.Length);$writer.Write([uint32]$offset)
  $offset+=$frame.Bytes.Length
 }
 foreach($frame in $frames){$writer.Write([byte[]]$frame.Bytes)}
}finally{$writer.Dispose();$stream.Dispose()}
