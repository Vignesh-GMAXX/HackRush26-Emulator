from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Optional

try:
    from .assembly import Instruction, Program, load_program, parse_memory_operand, resolve_local_label
except ImportError:  # pragma: no cover - direct script execution fallback
    from assembly import Instruction, Program, load_program, parse_memory_operand, resolve_local_label


def _s32(value: int) -> int:
    value &= 0xFFFFFFFF
    if value & 0x80000000:
        return value - 0x100000000
    return value


def _trunc_div(lhs: int, rhs: int) -> int:
    if rhs == 0:
        return -1
    return int(lhs / rhs)


def _trunc_rem(lhs: int, rhs: int) -> int:
    if rhs == 0:
        return lhs
    quotient = _trunc_div(lhs, rhs)
    return lhs - quotient * rhs


REG_ALIASES = {"zero": 0, "ra": 1, "sp": 2, "gp": 3, "tp": 4}
for index in range(8):
    REG_ALIASES[f"a{index}"] = 10 + index
for index in range(7):
    REG_ALIASES[f"t{index}"] = 5 + index if index < 3 else 28 + (index - 3)
for index in range(12):
    REG_ALIASES[f"s{index}"] = 8 + index if index < 2 else 16 + (index - 2)
for index in range(32):
    REG_ALIASES[f"x{index}"] = index


def _reg_index(token: str) -> int:
    token = token.strip()
    if token not in REG_ALIASES:
        raise ValueError(f"unknown register: {token}")
    return REG_ALIASES[token]


@dataclass
class ExecutionStats:
    instruction_count: int = 0
    total_cycles: int = 0
    active_cycles: int = 0
    sleep_cycles: int = 0
    radio_active_cycles: int = 0
    alu_cycles: int = 0
    load_cycles: int = 0
    store_cycles: int = 0
    branch_cycles: int = 0
    branch_penalty_cycles: int = 0
    load_use_stall_cycles: int = 0
    mul_div_cycles: int = 0
    memory_penalty_cycles: int = 0

    def clone(self) -> "ExecutionStats":
        return ExecutionStats(
            instruction_count=self.instruction_count,
            total_cycles=self.total_cycles,
            active_cycles=self.active_cycles,
            sleep_cycles=self.sleep_cycles,
            radio_active_cycles=self.radio_active_cycles,
            alu_cycles=self.alu_cycles,
            load_cycles=self.load_cycles,
            store_cycles=self.store_cycles,
            branch_cycles=self.branch_cycles,
            branch_penalty_cycles=self.branch_penalty_cycles,
            load_use_stall_cycles=self.load_use_stall_cycles,
            mul_div_cycles=self.mul_div_cycles,
            memory_penalty_cycles=self.memory_penalty_cycles,
        )

    def cpi(self) -> float:
        if self.instruction_count == 0:
            return 0.0
        return self.total_cycles / self.instruction_count

    def utilization_pct(self) -> float:
        if self.total_cycles == 0:
            return 0.0
        return (self.active_cycles * 100.0) / self.total_cycles

    def energy_mj(self, cycle_time_s: float, active_mw: float, sleep_mw: float, radio_mw: float) -> float:
        return (
            self.active_cycles * active_mw * cycle_time_s
            + self.sleep_cycles * sleep_mw * cycle_time_s
            + self.radio_active_cycles * radio_mw * cycle_time_s
        )

    def summary_lines(self) -> list[str]:
        return [
            "[PERFORMANCE-SUMMARY]",
            f"  instruction_count={self.instruction_count}",
            f"  total_cycles={self.total_cycles}",
            f"  active_cycles={self.active_cycles}",
            f"  sleep_cycles={self.sleep_cycles}",
            f"  radio_active_cycles={self.radio_active_cycles}",
            f"  average_cpi={self.cpi():.3f}",
            f"  cpu_utilization_pct={self.utilization_pct():.3f}",
            f"  alu_cycles={self.alu_cycles}",
            f"  load_cycles={self.load_cycles}",
            f"  store_cycles={self.store_cycles}",
            f"  branch_cycles={self.branch_cycles}",
            f"  branch_penalty_cycles={self.branch_penalty_cycles}",
            f"  load_use_stall_cycles={self.load_use_stall_cycles}",
            f"  mul_div_cycles={self.mul_div_cycles}",
            f"  memory_penalty_cycles={self.memory_penalty_cycles}",
        ]

    def power_lines(self, cycle_time_s: float, active_mw: float, sleep_mw: float, radio_mw: float) -> list[str]:
        return [
            "[POWER-MODEL]",
            f"  active_mw={active_mw:.2f}",
            f"  sleep_mw={sleep_mw:.2f}",
            f"  radio_extra_mw={radio_mw:.2f}",
            f"  cycle_time_s={cycle_time_s:.9f}",
            f"  estimated_energy_mj={self.energy_mj(cycle_time_s, active_mw, sleep_mw, radio_mw):.6f}",
        ]


@dataclass
class Snapshot:
    regs: list[int]
    memory: dict[int, int]
    pc: int
    halted: bool
    last_instruction: str | None
    last_load_dest: Optional[int]
    stats: ExecutionStats


class RiscVEmulator:
    PIPELINE_STAGES = 5
    ALU_CYCLES = 1
    BRANCH_CYCLES = 1
    BRANCH_TAKEN_PENALTY_CYCLES = 0
    LOAD_CYCLES = 1
    STORE_CYCLES = 1
    MUL_DIV_CYCLES = 1
    MEMORY_PENALTY_CYCLES = 0

    MEMORY_SIZE = 128 * 1024
    FAST_MEMORY_SIZE = 4 * 1024
    TEXT_START = 0x00000000
    TEXT_END = 0x00007FFF
    DATA_BSS_START = 0x00008000
    DATA_BSS_END = 0x0000FFFF
    STACK_START = 0x00010000
    STACK_END = MEMORY_SIZE - 1

    ACTIVE_POWER_MW = 20.0
    SLEEP_POWER_MW = 0.5
    RADIO_EXTRA_POWER_MW = 50.0
    CYCLE_TIME_S = 1.0e-6

    def __init__(self, source_path: Path) -> None:
        self.source_path = source_path
        self.program: Program = load_program(source_path)
        self.regs = [0] * 32
        self.memory: dict[int, int] = {}
        self.pc = 0
        self.trace = False
        self.halted = False
        self.last_instruction: str | None = None
        self._last_load_dest: Optional[int] = None
        self.stats = ExecutionStats()

    def _reset_stats(self) -> None:
        self.stats = ExecutionStats()
        self._last_load_dest = None

    def set_timing_model(self, *, mul_div_cycles: int | None = None, memory_penalty_cycles: int | None = None) -> None:
        if mul_div_cycles is not None:
            self.MUL_DIV_CYCLES = max(1, int(mul_div_cycles))
        if memory_penalty_cycles is not None:
            self.MEMORY_PENALTY_CYCLES = max(0, int(memory_penalty_cycles))

    def reset(self, function_name: str, args: Iterable[int] = ()) -> None:
        if function_name not in self.program.labels:
            raise ValueError(f"entry label not found: {function_name}")

        self.regs = [0] * 32
        self.memory = {}
        self.pc = self.program.labels[function_name]
        self.halted = False
        self.last_instruction = None
        self._reset_stats()
        self.regs[_reg_index("sp")] = self.STACK_END - 3
        self.regs[_reg_index("ra")] = -1

        for index, value in enumerate(args):
            if index >= 8:
                break
            self.regs[_reg_index(f"a{index}")] = _s32(int(value))

    def add_sleep_cycles(self, cycles: int) -> None:
        if cycles <= 0:
            return
        self.stats.sleep_cycles += cycles
        self.stats.total_cycles += cycles

    def add_radio_active_cycles(self, cycles: int) -> None:
        if cycles <= 0:
            return
        self.stats.radio_active_cycles += cycles

    def _get_reg(self, name: str) -> int:
        return self.regs[_reg_index(name)]

    def _get_value(self, token: str) -> int:
        token = token.strip()
        if token in REG_ALIASES:
            return self._get_reg(token)
        return int(token, 0)

    def _set_reg(self, name: str, value: int) -> None:
        index = _reg_index(name)
        if index == 0:
            return
        self.regs[index] = _s32(value)

    def _check_memory_bounds(self, address: int) -> None:
        if address < 0 or address + 4 > self.MEMORY_SIZE:
            raise MemoryError(f"memory access out of bounds: 0x{address:08x}")

    def _mem_load(self, address: int) -> int:
        self._check_memory_bounds(address)
        return _s32(self.memory.get(address, 0))

    def _mem_store(self, address: int, value: int) -> None:
        self._check_memory_bounds(address)
        self.memory[address] = _s32(value)

    def _memory_penalty(self, address: int) -> int:
        return 0 if address < self.FAST_MEMORY_SIZE else self.MEMORY_PENALTY_CYCLES

    def _register_hazard(self, instruction: Instruction, previous_load_dest: Optional[int]) -> bool:
        if previous_load_dest is None or not instruction.operands:
            return False

        opcode = instruction.opcode
        sources: list[str] = []
        if opcode in {"add", "sub", "ori", "mul", "div", "rem", "slt"}:
            sources = instruction.operands[1:3]
        elif opcode in {"addi", "mv"}:
            sources = instruction.operands[1:2]
        elif opcode == "sw":
            sources = [instruction.operands[0], parse_memory_operand(instruction.operands[1])[1]]
        elif opcode == "lw":
            sources = [parse_memory_operand(instruction.operands[1])[1]]
        elif opcode in {"beq", "bne", "blt", "bge"}:
            sources = instruction.operands[0:2]
        elif opcode in {"bnez", "beqz"}:
            sources = instruction.operands[0:1]

        for token in sources:
            if token in REG_ALIASES and _reg_index(token) == previous_load_dest:
                return True
        return False

    def _instruction_cost(self, instruction: Instruction, jump_target: Optional[int], previous_load_dest: Optional[int]) -> int:
        opcode = instruction.opcode
        base_cost = 1
        penalty = 0

        if opcode in {"mul", "div", "rem"}:
            base_cost = self.MUL_DIV_CYCLES
            self.stats.mul_div_cycles += base_cost
        elif opcode == "lw":
            offset, base = parse_memory_operand(instruction.operands[1])
            address = self._get_reg(base) + offset
            base_cost = self.LOAD_CYCLES
            mem_penalty = self._memory_penalty(address)
            penalty += mem_penalty
            self.stats.load_cycles += base_cost
            self.stats.memory_penalty_cycles += mem_penalty
        elif opcode == "sw":
            offset, base = parse_memory_operand(instruction.operands[1])
            address = self._get_reg(base) + offset
            base_cost = self.STORE_CYCLES
            mem_penalty = self._memory_penalty(address)
            penalty += mem_penalty
            self.stats.store_cycles += base_cost
            self.stats.memory_penalty_cycles += mem_penalty
        elif opcode in {"beq", "bne", "blt", "bge", "bnez", "beqz", "j", "jal", "ret"}:
            base_cost = self.BRANCH_CYCLES
            self.stats.branch_cycles += base_cost
            if jump_target is not None:
                penalty += self.BRANCH_TAKEN_PENALTY_CYCLES
                self.stats.branch_penalty_cycles += self.BRANCH_TAKEN_PENALTY_CYCLES
        else:
            self.stats.alu_cycles += base_cost

        if self._register_hazard(instruction, previous_load_dest):
            penalty += 1
            self.stats.load_use_stall_cycles += 1

        return base_cost + penalty

    def run(self, function_name: str, args: Iterable[int], trace: bool = False) -> int:
        self.reset(function_name, args)
        self.trace = trace

        steps = 0
        while self.step():
            steps += 1
            if steps > 100000:
                raise RuntimeError("execution limit exceeded")

        return self.regs[_reg_index("a0")]

    def snapshot(self) -> Snapshot:
        return Snapshot(
            regs=self.regs.copy(),
            memory=self.memory.copy(),
            pc=self.pc,
            halted=self.halted,
            last_instruction=self.last_instruction,
            last_load_dest=self._last_load_dest,
            stats=self.stats.clone(),
        )

    def restore(self, snapshot: Snapshot) -> None:
        self.regs = snapshot.regs.copy()
        self.memory = snapshot.memory.copy()
        self.pc = snapshot.pc
        self.halted = snapshot.halted
        self.last_instruction = snapshot.last_instruction
        self._last_load_dest = snapshot.last_load_dest
        self.stats = snapshot.stats.clone()

    def step(self) -> bool:
        if self.halted:
            return False
        if not (0 <= self.pc < len(self.program.instructions)):
            self.halted = True
            return False

        instruction = self.program.instructions[self.pc]
        previous_load_dest = self._last_load_dest
        next_pc = self.pc + 1

        if self.trace:
            self._trace(instruction)

        jump_target = self._execute(instruction)
        cost = self._instruction_cost(instruction, jump_target, previous_load_dest)

        # Model pipeline fill latency once at the first issued instruction.
        if self.stats.instruction_count == 0:
            self.stats.total_cycles += max(0, self.PIPELINE_STAGES - 1)

        self.stats.instruction_count += 1
        self.stats.active_cycles += cost
        self.stats.total_cycles += cost

        self.last_instruction = f"{instruction.line_number}:{instruction.opcode} {' '.join(instruction.operands)}".strip()
        if jump_target is not None:
            next_pc = jump_target
        self.pc = next_pc

        if not (0 <= self.pc < len(self.program.instructions)):
            self.halted = True
        return not self.halted

    def _trace(self, instruction: Instruction) -> None:
        operands = ", ".join(instruction.operands)
        print(f"{instruction.line_number:04d}: {instruction.opcode} {operands}")

    def _resolve_label(self, token: str) -> int:
        if token in self.program.labels:
            return self.program.labels[token]
        return resolve_local_label(self.program.label_positions, self.pc, token)

    def _execute(self, instruction: Instruction) -> Optional[int]:
        op = instruction.opcode
        operands = instruction.operands

        if op in {".text", ".align", ".globl"}:
            return None
        if op == "li":
            self._set_reg(operands[0], int(operands[1], 0))
        elif op == "mv":
            self._set_reg(operands[0], self._get_reg(operands[1]))
        elif op == "addi":
            self._set_reg(operands[0], self._get_reg(operands[1]) + int(operands[2], 0))
        elif op == "add":
            self._set_reg(operands[0], self._get_value(operands[1]) + self._get_value(operands[2]))
        elif op == "sub":
            self._set_reg(operands[0], self._get_value(operands[1]) - self._get_value(operands[2]))
        elif op == "ori":
            self._set_reg(operands[0], self._get_value(operands[1]) | self._get_value(operands[2]))
        elif op == "mul":
            self._set_reg(operands[0], _s32(self._get_value(operands[1]) * self._get_value(operands[2])))
        elif op == "div":
            self._set_reg(operands[0], _trunc_div(self._get_value(operands[1]), self._get_value(operands[2])))
        elif op == "rem":
            self._set_reg(operands[0], _trunc_rem(self._get_value(operands[1]), self._get_value(operands[2])))
        elif op == "slt":
            self._set_reg(operands[0], 1 if self._get_value(operands[1]) < self._get_value(operands[2]) else 0)
        elif op == "sw":
            src = self._get_reg(operands[0])
            offset, base = parse_memory_operand(operands[1])
            self._mem_store(self._get_reg(base) + offset, src)
        elif op == "lw":
            offset, base = parse_memory_operand(operands[1])
            address = self._get_reg(base) + offset
            self._set_reg(operands[0], self._mem_load(address))
            self._last_load_dest = _reg_index(operands[0])
        elif op == "jal":
            if len(operands) == 1:
                self._set_reg("ra", self.pc + 1)
                return self._resolve_label(operands[0])
            if len(operands) == 2:
                self._set_reg(operands[0], self.pc + 1)
                return self._resolve_label(operands[1])
            raise ValueError(f"unsupported jal form at line {instruction.line_number}")
        elif op == "j":
            return self._resolve_label(operands[0])
        elif op == "ret":
            return self.regs[_reg_index("ra")]
        elif op == "bnez":
            if self._get_reg(operands[0]) != 0:
                return self._resolve_label(operands[1])
        elif op == "beqz":
            if self._get_reg(operands[0]) == 0:
                return self._resolve_label(operands[1])
        elif op == "beq":
            if self._get_reg(operands[0]) == self._get_reg(operands[1]):
                return self._resolve_label(operands[2])
        elif op == "bne":
            if self._get_reg(operands[0]) != self._get_reg(operands[1]):
                return self._resolve_label(operands[2])
        elif op == "blt":
            if self._get_reg(operands[0]) < self._get_reg(operands[1]):
                return self._resolve_label(operands[2])
        elif op == "bge":
            if self._get_reg(operands[0]) >= self._get_reg(operands[1]):
                return self._resolve_label(operands[2])
        else:
            raise ValueError(f"unsupported opcode '{op}' at line {instruction.line_number}")

        if op != "lw":
            self._last_load_dest = None
        return None

    def listing_text(self) -> str:
        lines = []
        for index, instruction in enumerate(self.program.instructions):
            marker = "CURR->" if index == self.pc else "      "
            operands = ", ".join(instruction.operands)
            lines.append(f"{marker}{index}:{instruction.opcode} {operands}".rstrip())
        return "\n".join(lines)

    def state_text(self) -> str:
        reg_names = [
            "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
            "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6",
        ]

        lines = [f"PC={self.pc}", f"HALTED={self.halted}"]
        if self.last_instruction:
            lines.append(f"LAST={self.last_instruction}")

        lines.append("")
        lines.append("[ABSTRACT-MEMORY-MODEL]")
        lines.append(f"  total_ram_bytes={self.MEMORY_SIZE}")
        lines.append(f"  fast_memory_bytes={self.FAST_MEMORY_SIZE}")
        lines.append(f"  text_region=0x{self.TEXT_START:08x}-0x{self.TEXT_END:08x}")
        lines.append(f"  data_bss_region=0x{self.DATA_BSS_START:08x}-0x{self.DATA_BSS_END:08x}")
        lines.append(f"  stack_region=0x{self.STACK_START:08x}-0x{self.STACK_END:08x}")
        lines.append(f"  stack_pointer=0x{self._get_reg('sp') & 0xffffffff:08x}")

        lines.append("")
        lines.append("REGISTERS")
        for index, name in enumerate(reg_names):
            lines.append(f"R{index:02d} ({name:>4}) = {self.regs[index]:>11}")

        if self.memory:
            lines.append("")
            lines.append("MEMORY")
            for address in sorted(self.memory)[:32]:
                lines.append(f"[{address:08x}] = {self.memory[address]}")

        return "\n".join(lines)

    def performance_text(self) -> str:
        lines = [
            f"[PERFORMANCE-SUMMARY]",
            f"  instruction_count={self.stats.instruction_count}",
            f"  total_cycles={self.stats.total_cycles}",
            f"  average_cpi={self.stats.cpi():.3f}",
            f"  stall_cycles={self.stats.load_use_stall_cycles}",
        ]
        lines.append("")
        lines.append(f"[POWER-MODEL]")
        lines.append(f"  estimated_energy_mj={self.stats.energy_mj(self.CYCLE_TIME_S, self.ACTIVE_POWER_MW, self.SLEEP_POWER_MW, self.RADIO_EXTRA_POWER_MW):.6f}")
        return "\n".join(lines)
