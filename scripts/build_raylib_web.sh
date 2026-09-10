#!/usr/bin/env bash
# Regenerates lib/raylib-web/ (the raylib static library built for
# PLATFORM_WEB with emcc), the same way lib/raylib-linux and
# lib/raylib-win64 were produced for their respective platforms.
#
# Requirements:
#   - Emscripten SDK installed and activated (emcc on PATH, or run
#     `source /path/to/emsdk/emsdk_env.sh` first).
#   - curl, tar.
#
# Usage:
#   ./scripts/build_raylib_web.sh [raylib_version]
#
# The raylib version defaults to 5.5, matching lib/raylib-linux and
# lib/raylib-win64. Re-run this script (with a different version, or after
# an emsdk update) any time lib/raylib-web/ needs to be regenerated; the
# output directory is checked into the repo like the other lib/raylib-*
# directories, so this script is documentation more than a required build
# step for day-to-day work.

set -euo pipefail

RAYLIB_VERSION="${1:-5.5}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST_DIR="$REPO_ROOT/lib/raylib-web"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

if ! command -v emcc >/dev/null 2>&1; then
  echo "error: emcc not found on PATH. Activate the Emscripten SDK first" \
       "(source \$EMSDK/emsdk_env.sh)." >&2
  exit 1
fi

echo "Downloading raylib $RAYLIB_VERSION source..."
curl -sL -o "$WORK_DIR/raylib.tar.gz" \
  "https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz"
tar xzf "$WORK_DIR/raylib.tar.gz" -C "$WORK_DIR"

SRC_DIR="$WORK_DIR/raylib-${RAYLIB_VERSION}"

echo "Building libraylib.a for PLATFORM_WEB..."
emmake make -C "$SRC_DIR/src" PLATFORM=PLATFORM_WEB -B

echo "Installing into $DEST_DIR ..."
rm -rf "$DEST_DIR"
mkdir -p "$DEST_DIR/include" "$DEST_DIR/lib"
cp "$SRC_DIR/src/libraylib.a" "$DEST_DIR/lib/libraylib.a"
cp "$SRC_DIR/src/raylib.h" "$SRC_DIR/src/raymath.h" "$SRC_DIR/src/rlgl.h" "$DEST_DIR/include/"
cp "$SRC_DIR/CHANGELOG" "$SRC_DIR/LICENSE" "$SRC_DIR/README.md" "$DEST_DIR/"

echo "Done. lib/raylib-web/ is ready for 'make web'."
