from __future__ import annotations

from dataclasses import dataclass
import re
from pathlib import Path
from typing import Dict, List


@dataclass(frozen=True)
class Instruction:
    opcode: str
    operands: List[str]
    line_number: int


@dataclass(frozen=True)
class Program:
    instructions: List[Instruction]
    labels: Dict[str, int]
    label_positions: Dict[str, List[int]]


def load_program(source_path: Path) -> Program:
    instructions: List[Instruction] = []
    labels: Dict[str, int] = {}
    label_positions: Dict[str, List[int]] = {}
    current_index = 0

    for line_number, raw_line in enumerate(source_path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.split("#", 1)[0].strip()
        if not line:
            continue

        while ":" in line:
            label_part, remainder = line.split(":", 1)
            label = label_part.strip()
            if label:
                labels[label] = current_index
                label_positions.setdefault(label, []).append(current_index)
            line = remainder.strip()
            if not line:
                break

        if not line:
            continue
        if line.startswith("."):
            continue

        opcode, *rest = line.split(None, 1)
        operands = []
        if rest:
            operands = [item.strip() for item in rest[0].split(",")]
        instructions.append(Instruction(opcode.lower(), operands, line_number))
        current_index += 1

    return Program(instructions=instructions, labels=labels, label_positions=label_positions)


def parse_memory_operand(token: str) -> tuple[int, str]:
    match = re.fullmatch(r"(-?\d+)\(([^)]+)\)", token.replace(" ", ""))
    if not match:
        raise ValueError(f"invalid memory operand: {token}")
    return int(match.group(1), 10), match.group(2)


def resolve_local_label(label_positions: Dict[str, List[int]], current_pc: int, token: str) -> int:
    match = re.fullmatch(r"(\d+)([fb])", token)
    if not match:
        raise ValueError(f"unsupported label format: {token}")

    base = match.group(1)
    direction = match.group(2)
    positions = label_positions.get(base, [])
    if not positions:
        raise ValueError(f"unknown local label: {token}")

    if direction == "f":
        for position in positions:
            if position > current_pc:
                return position
    else:
        for position in reversed(positions):
            if position < current_pc:
                return position

    raise ValueError(f"unresolved local label: {token}")
