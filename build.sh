#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "[build] Building Part 1 simulator..."
mkdir -p "$ROOT_DIR/simulator_core/bin"
cc -std=c11 -O2 -Wall -Wextra -I"$ROOT_DIR/simulator_core/include" \
  "$ROOT_DIR/simulator_core/src/main.c" \
  "$ROOT_DIR/simulator_core/src/runtime.c" \
  "$ROOT_DIR/simulator_core/src/scenario.c" \
  "$ROOT_DIR/simulator_core/src/tasks.c" \
  -o "$ROOT_DIR/simulator_core/bin/sat_sim"

echo "[build] Build complete: simulator_core/bin/sat_sim"
