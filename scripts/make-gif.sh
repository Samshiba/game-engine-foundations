#!/usr/bin/env bash
# Turns an F11 capture (200 frames at a fixed 30 fps step) into a GIF, with a
# palette computed from the frames so the colors stay clean.
#
# Usage: scripts/make-gif.sh <capture dir> [output.gif] [width]
#   scripts/make-gif.sh bin/screenshots/20261119-104603
#   scripts/make-gif.sh bin/screenshots/20261119-104603 demo.gif 720
#
# Needs ffmpeg.

set -euo pipefail

if [[ $# -lt 1 ]]; then
    sed -n '2,9p' "$0"
    exit 1
fi

capture_dir=$1
output=${2:-demo.gif}
width=${3:-960}

if [[ ! -f $capture_dir/frame_0000.png ]]; then
    echo "No frame_0000.png in $capture_dir" >&2
    exit 1
fi

ffmpeg -y -loglevel error -framerate 30 -i "$capture_dir/frame_%04d.png" \
    -vf "fps=30,scale=$width:-1:flags=lanczos,split[a][b];[a]palettegen[p];[b][p]paletteuse" \
    "$output"
echo "Saved $output"
