#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "[build] Building Part 1 simulator..."
mkdir -p "$ROOT_DIR/part1/bin"
cc -std=c11 -O2 -Wall -Wextra -I"$ROOT_DIR/part1/include" \
  "$ROOT_DIR/part1/src/main.c" \
  "$ROOT_DIR/part1/src/runtime.c" \
  "$ROOT_DIR/part1/src/scenario.c" \
  "$ROOT_DIR/part1/src/tasks.c" \
  -o "$ROOT_DIR/part1/bin/sat_sim"

echo "[build] Build complete: part1/bin/sat_sim"
