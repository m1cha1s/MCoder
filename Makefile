ifeq ($(OS),Windows_NT)   # WARNING: Not tested.
    CC := cl
    MAKE := nmake
    # This is surely wrong.
    CFLAGS := -O2 -g -I raylib/src
else
    detected_OS := $(shell uname)  # same as "uname -s"

    CC := cc
    MAKE := make
    CFLAGS := -fsanitize=address -O2 -g -I raylib/src

    ifneq (,$(findstring Darwin,$(detected_OS)))
        LDFLAGS := -framework Cocoa -framework IOKit -framework CoreFoundation
    else
        LDFLAGS := -lGL -lm -lpthread -ldl -lrt -lX11
    endif
endif


EXE := MCoder

SRC := $(shell find src -maxdepth 1 -name "*.c")

$(EXE): $(SRC) raylib/src/libraylib.a
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

raylib/src/libraylib.a: raylib/src
	$(MAKE) -C raylib/src PLATFORM=PLATFORM_DESKTOP -j

raylib/src:
	git submodule update --init --recursive

.PHONY: clean build run

build: $(EXE)

run: $(EXE)
	./$(EXE)

clean:
	rm -rf $(EXE) *.dSYM
