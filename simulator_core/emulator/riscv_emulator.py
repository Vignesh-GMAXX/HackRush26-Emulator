#!/usr/bin/env python3
"""Backward-compatible entrypoint for the RISC-V emulator."""

from cli import main


if __name__ == "__main__":
    raise SystemExit(main())