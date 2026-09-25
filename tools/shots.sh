#!/usr/bin/env bash
#
# Refresh assets/screenshots/ -- the scenes and their quirks live here rather
# than in the Makefile, where shell quoting and make 3.81's pattern-rule
# behaviour made a mess of them.
#
# Each platform gets the tour scene, written straight into the assets folder
# (that is screenshots.sh's --output, which it wipes first). The four colour
# platforms then get the picker scene too, and its three shots are copied in
# beside the tour's: the picker scene runs into tmp/, because screenshots.sh
# wipes --output, and pointing it at the assets folder would erase the tour.
#
# Usage:
#   tools/shots.sh                 all six platforms
#   tools/shots.sh basalt          just these platforms
#   tools/shots.sh -p basalt       ditto (repeatable)
#   tools/shots.sh check           diff the empty menu against the upstream
#                                  baselines (aplite, basalt, chalk)
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ALL=(aplite basalt chalk diorite emery gabbro)
COLOUR=(basalt chalk emery gabbro)
BASELINED=(aplite basalt chalk)
TOUR="$ROOT/tools/scenes/readme-tour.scene"
TOUR_BW="$ROOT/tools/scenes/readme-aplite.scene"
COLOR="$ROOT/tools/scenes/color-picker.scene"
EMPTY="$ROOT/tools/scenes/menu-empty.scene"
SHOOTER="$ROOT/tools/screenshots.sh"
ASSETS="$ROOT/assets/screenshots"
BASEDIR="$ROOT/tmp/baselines/upstream-master"

plats=()
check=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    check) check=1; shift ;;
    -p|--platform) plats+=("$2"); shift 2 ;;
    -h|--help) sed -n '/^# Usage:/,/^$/p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    -*) echo "unknown option $1" >&2; exit 1 ;;
    *) plats+=("$1"); shift ;;
  esac
done

has() { local x="$1"; shift; [[ " $* " == *" $x "* ]]; }

if [[ $check -eq 1 ]]; then
  for plat in "${BASELINED[@]}"; do
    "$SHOOTER" -p "$plat" -w -b "$BASEDIR/$plat" -o "$ROOT/tmp/shots/check/$plat" "$EMPTY" || exit 1
  done
  exit 0
fi

[[ ${#plats[@]} -gt 0 ]] || plats=("${ALL[@]}")

for plat in "${plats[@]}"; do
  has "$plat" "${ALL[@]}" || { echo "unknown platform '$plat' -- try: ${ALL[*]}" >&2; exit 1; }
  scene=$TOUR
  has "$plat" aplite && scene=$TOUR_BW
  "$SHOOTER" -p "$plat" -w -o "$ASSETS/$plat" "$scene"
  if has "$plat" "${COLOUR[@]}"; then
    "$SHOOTER" -p "$plat" -w "$COLOR"
    cp "$ROOT"/tmp/shots/color-picker/"$plat"/{01-color-picker,02-color-chosen,03-color-list}.png \
      "$ASSETS/$plat/"
  fi
done
