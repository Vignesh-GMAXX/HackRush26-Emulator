CC = gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra
INCLUDES = -Iinclude
SRCS = src/main.c src/runtime.c src/scenario.c src/tasks.c
OUT = bin/sat_sim.exe
CLEAN_OUT = bin\sat_sim.exe
RUN_OUT = bin\sat_sim.exe
SCENARIO = scenarios\conic_scenario_dataset.json
LOG = ..\results\conic_scenario_dataset.log

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

.PHONY: all clean run run-log
