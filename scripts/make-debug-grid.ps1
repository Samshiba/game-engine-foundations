# Builds a 2x2 grid of debug views from an F10 capture.
#
# F10 saves one frame per debug view, in the order of SceneRenderer::DebugView:
#   0 None, 1 Albedo, 2 Normal, 3 UV, 4 Tangent, 5 Normal map, 6 Lighting only
#
# Usage: scripts\make-debug-grid.ps1 <capture dir> [output.png] [4 view indices]
#   scripts\make-debug-grid.ps1 bin\screenshots\20261119-104603
#   scripts\make-debug-grid.ps1 bin\screenshots\20261119-104603 grid.png 1 5 4 6
#
# Needs ffmpeg. Set $env:FONT to a .ttf file to pick the label font.

param(
    [Parameter(Mandatory = $true)][string]$CaptureDir,
    [string]$Output = "debug-views.png",
    [Parameter(ValueFromRemainingArguments = $true)][int[]]$Views
)

$ErrorActionPreference = "Stop"

if (-not $Views) { $Views = @(1, 5, 3, 6) }
if ($Views.Count -ne 4) {
    throw "Expected 4 views, got $($Views.Count)"
}

$labels = @("None", "Albedo", "Normal", "UV", "Tangent", "Normal map", "Lighting only")

$font = if ($env:FONT) { $env:FONT } else { "C:/Windows/Fonts/arial.ttf" }
$font = $font -replace "\\", "/" -replace ":", "\:" # drawtext needs the drive colon escaped

$inputs = @()
$filters = ""
for ($i = 0; $i -lt 4; $i++) {
    $view = $Views[$i]
    $frame = Join-Path $CaptureDir ("frame_{0:D4}.png" -f $view)
    if (-not (Test-Path $frame)) {
        throw "Missing $frame"
    }
    $inputs += @("-i", $frame)
    $filters += "[$i]scale=640:-1,drawtext=text='$($labels[$view])':fontfile='$font'"
    $filters += ":x=12:y=12:fontsize=22:fontcolor=white:box=1:boxcolor=black@0.6:boxborderw=6[t$i];"
}
$filters += "[t0][t1][t2][t3]xstack=inputs=4:layout=0_0|w0_0|0_h0|w0_h0"

& ffmpeg -y -loglevel error @inputs -filter_complex $filters $Output
if ($LASTEXITCODE -ne 0) {
    throw "ffmpeg failed with exit code $LASTEXITCODE"
}
Write-Host "Saved $Output"
