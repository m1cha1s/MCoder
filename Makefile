_dummy := $(shell mkdir -p build)

CC := cc
AR := ar

CFLAGS := -O0 -g -I glfw/include
LDFLAGS := -framework Cocoa -framework IOKit -framework CoreFoundation

EXE := MCoder

SRC := $(shell find . -depth 1 -name "*.c")

$(EXE): $(SRC) build/glfw.a
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

include glfw.mk

.PHONY: clean build run

build: $(EXE)
