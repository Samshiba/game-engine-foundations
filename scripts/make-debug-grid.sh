#!/usr/bin/env bash
# Builds a 2x2 grid of debug views from an F10 capture.
#
# F10 saves one frame per debug view, in the order of SceneRenderer::DebugView:
#   0 None, 1 Albedo, 2 Normal, 3 UV, 4 Tangent, 5 Normal map, 6 Lighting only
#
# Usage: scripts/make-debug-grid.sh <capture dir> [output.png] [4 view indices]
#   scripts/make-debug-grid.sh bin/screenshots/20261119-104603
#   scripts/make-debug-grid.sh bin/screenshots/20261119-104603 grid.png 1 5 4 6
#
# Needs ffmpeg. Set FONT to a .ttf file to pick the label font.

set -euo pipefail

if [[ $# -lt 1 ]]; then
    sed -n '2,11p' "$0"
    exit 1
fi

capture_dir=$1
output=${2:-debug-views.png}
shift $(( $# >= 2 ? 2 : 1 ))
views=("$@")
[[ ${#views[@]} -eq 0 ]] && views=(1 5 3 6)

if [[ ${#views[@]} -ne 4 ]]; then
    echo "Expected 4 views, got ${#views[@]}" >&2
    exit 1
fi

labels=("None" "Albedo" "Normal" "UV" "Tangent" "Normal map" "Lighting only")

if [[ -z ${FONT:-} ]]; then
    case "$(uname -s)" in
        MINGW* | MSYS* | CYGWIN*) FONT="C:/Windows/Fonts/arial.ttf" ;;
        Darwin) FONT="/System/Library/Fonts/Helvetica.ttc" ;;
        *) FONT=$(fc-match -f '%{file}' sans) ;;
    esac
fi
font=${FONT//:/\\:} # drawtext needs the drive colon escaped

inputs=()
filters=""
for i in 0 1 2 3; do
    view=${views[$i]}
    frame=$(printf "%s/frame_%04d.png" "$capture_dir" "$view")
    if [[ ! -f $frame ]]; then
        echo "Missing $frame" >&2
        exit 1
    fi
    inputs+=(-i "$frame")
    filters+="[$i]scale=640:-1,drawtext=text='${labels[$view]}':fontfile='$font'"
    filters+=":x=12:y=12:fontsize=22:fontcolor=white:box=1:boxcolor=black@0.6:boxborderw=6[t$i];"
done
filters+="[t0][t1][t2][t3]xstack=inputs=4:layout=0_0|w0_0|0_h0|w0_h0"

ffmpeg -y -loglevel error "${inputs[@]}" -filter_complex "$filters" "$output"
echo "Saved $output"
