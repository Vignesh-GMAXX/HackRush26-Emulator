CC = gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra
INCLUDES = -Iinclude
SRCS = src/main.c src/runtime.c src/scenario.c src/tasks.c
OUT = bin/sat_sim.exe
CLEAN_OUT = bin\sat_sim.exe
RUN_OUT = bin\sat_sim.exe
SCENARIO = scenarios\conic_scenario_dataset.json
LOG = ..\results\conic_scenario_dataset.log
PYTHON ?= python
EMU_ASM = asm\runtime_kernels_riscv32.s
EMU_FUNC ?= rv_risk_classify
EMU_ARGS ?= 120 3
GUI_ENTRY = tools\gui.py

all: $(OUT)

$(OUT): $(SRCS)
	if not exist bin mkdir bin
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) -o $(OUT)

clean:
	if exist $(CLEAN_OUT) del /q $(CLEAN_OUT)

run: $(OUT)
	$(RUN_OUT) $(SCENARIO)

run-log: $(OUT)
	$(RUN_OUT) $(SCENARIO) > $(LOG)

emu:
	$(PYTHON) tools\riscv_emulator.py $(EMU_ASM) $(EMU_FUNC) $(EMU_ARGS)

gui:
	$(PYTHON) $(GUI_ENTRY)

.PHONY: all clean run run-log emu gui
