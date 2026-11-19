# Turns an F11 capture (200 frames at a fixed 30 fps step) into a GIF, with a
# palette computed from the frames so the colors stay clean.
#
# Usage: scripts\make-gif.ps1 <capture dir> [output.gif] [width]
#   scripts\make-gif.ps1 bin\screenshots\20261119-104603
#   scripts\make-gif.ps1 bin\screenshots\20261119-104603 demo.gif 720
#
# Needs ffmpeg.

param(
    [Parameter(Mandatory = $true)][string]$CaptureDir,
    [string]$Output = "demo.gif",
    [int]$Width = 960
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path (Join-Path $CaptureDir "frame_0000.png"))) {
    throw "No frame_0000.png in $CaptureDir"
}

& ffmpeg -y -loglevel error -framerate 30 -i (Join-Path $CaptureDir "frame_%04d.png") `
    -vf "fps=30,scale=${Width}:-1:flags=lanczos,split[a][b];[a]palettegen[p];[b][p]paletteuse" `
    $Output
if ($LASTEXITCODE -ne 0) {
    throw "ffmpeg failed with exit code $LASTEXITCODE"
}
Write-Host "Saved $Output"
