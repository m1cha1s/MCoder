CC := cc

CFLAGS := -O2 -g -I raylib/src
LDFLAGS := -framework Cocoa -framework IOKit -framework CoreFoundation

EXE := MCoder

SRC := $(shell find . -depth 1 -name "*.c")

$(EXE): $(SRC) raylib/src/libraylib.a
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

raylib/src/libraylib.a: raylib
	make -C raylib/src PLATFORM=PLATFORM_DESKTOP -j

raylib:
	git submodule update --init --recursive

.PHONY: clean build run

build: $(EXE)

run: $(EXE)
	./$(EXE)

clean:
	rm -f $(EXE)
