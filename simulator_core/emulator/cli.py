from __future__ import annotations

import argparse
from pathlib import Path

try:
    from .emulator import RiscVEmulator
except ImportError:  # pragma: no cover - direct script execution fallback
    from emulator import RiscVEmulator


def main() -> int:
    parser = argparse.ArgumentParser(description="Run RV32IM helper assembly in the Python emulator.")
    parser.add_argument("asm_file", type=Path, help="Path to the RISC-V assembly file")
    parser.add_argument("function", help="Label of the function to execute")
    parser.add_argument("args", nargs="*", type=int, help="Integer arguments passed in a0-a7")
    parser.add_argument("--trace", action="store_true", help="Print each executed instruction")
    parsed = parser.parse_args()

    emulator = RiscVEmulator(parsed.asm_file)
    result = emulator.run(parsed.function, parsed.args, trace=parsed.trace)
    print(result)
    print(emulator.performance_text())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
