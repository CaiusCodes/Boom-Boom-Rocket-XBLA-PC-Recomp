[CmdletBinding()]
param([string]$SdkDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if(-not $SdkDirectory){$SdkDirectory=Join-Path $root 'out/dependencies/rexglue-sdk'}
$destination=Join-Path $root 'packaging/licenses'
$licenses=[ordered]@{
 'LICENSE-Project.txt'=(Join-Path $root 'LICENSE')
 'LICENSE-ReXGlue.txt'=(Join-Path $SdkDirectory 'LICENSE')
 'LICENSE-FFmpeg-LGPL.txt'=(Join-Path $SdkDirectory 'thirdparty/FFmpeg/COPYING.LGPLv2.1')
 'LICENSE-libmspack.txt'=(Join-Path $SdkDirectory 'thirdparty/libmspack/libmspack/COPYING.LIB')
 'LICENSE-SDL3.txt'=(Join-Path $SdkDirectory 'thirdparty/sdl3/LICENSE.txt')
 'LICENSE-Tracy.txt'=(Join-Path $SdkDirectory 'thirdparty/tracy/LICENSE')
 'LICENSE-fmt.txt'=(Join-Path $SdkDirectory 'thirdparty/fmt/LICENSE')
 'LICENSE-spdlog.txt'=(Join-Path $SdkDirectory 'thirdparty/spdlog/LICENSE')
 'LICENSE-snappy.txt'=(Join-Path $SdkDirectory 'thirdparty/snappy/COPYING')
 'LICENSE-imgui.txt'=(Join-Path $SdkDirectory 'thirdparty/imgui/LICENSE.txt')
 'LICENSE-tomlplusplus.txt'=(Join-Path $SdkDirectory 'thirdparty/tomlplusplus/LICENSE')
 'LICENSE-simde.txt'=(Join-Path $SdkDirectory 'thirdparty/simde/COPYING')
 'LICENSE-utfcpp.txt'=(Join-Path $SdkDirectory 'thirdparty/utfcpp/LICENSE')
 'LICENSE-xxHash.txt'=(Join-Path $SdkDirectory 'thirdparty/xxHash/LICENSE')
 'LICENSE-o1heap.txt'=(Join-Path $SdkDirectory 'thirdparty/o1heap/LICENSE')
 'LICENSE-aes128.txt'=(Join-Path $SdkDirectory 'thirdparty/aes_128/LICENSE')
 'LICENSE-CLI11.txt'=(Join-Path $SdkDirectory 'thirdparty/cli11/LICENSE')
 'LICENSE-inja.txt'=(Join-Path $SdkDirectory 'thirdparty/inja/LICENSE')
}
foreach($name in $licenses.Keys){Copy-Item -LiteralPath $licenses[$name] -Destination (Join-Path $destination $name)}
$renderdoc=Get-Content -LiteralPath (Join-Path $SdkDirectory 'thirdparty/renderdoc/renderdoc_app.h') -Raw
$notice=[regex]::Match($renderdoc,'(?s)^/\*+(.*?)\*+/').Groups[1].Value
if(-not $notice.Contains('Baldur Karlsson')){throw 'RenderDoc notice extraction failed'}
[IO.File]::WriteAllText((Join-Path $destination 'LICENSE-RenderDoc.txt'),$notice,[Text.UTF8Encoding]::new($false))
Write-Host "Copied $($licenses.Count) upstream/project license files. Existing extractor notices retained."
