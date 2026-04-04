CC = gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra
INCLUDES = -Iinclude
SRCS = src/main.c src/runtime.c src/scenario.c src/tasks.c
OUT = bin/sat_sim.exe
CLEAN_OUT = bin\sat_sim.exe

all: $(OUT)

$(OUT): $(SRCS)
	if not exist bin mkdir bin
	$(CC) $(CFLAGS) $(INCLUDES) $(SRCS) -o $(OUT)

clean:
	if exist $(CLEAN_OUT) del /q $(CLEAN_OUT)

.PHONY: all clean
