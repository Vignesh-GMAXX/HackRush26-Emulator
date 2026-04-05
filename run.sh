#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCENARIO_PATH="${1:-$ROOT_DIR/simulator_core/scenarios/simple_nominal_scenario.json}"

if [[ ! -x "$ROOT_DIR/simulator_core/bin/sat_sim" ]]; then
  echo "[run] Binary missing, invoking build.sh first..."
  "$ROOT_DIR/build.sh"
fi

"$ROOT_DIR/simulator_core/bin/sat_sim" "$SCENARIO_PATH"
