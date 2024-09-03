ifeq ($(OS),Windows_NT)   # WARNING: Not tested.
    CC := cl
    MAKE := nmake
    # This is surely wrong.
    CFLAGS := -O2 -g -I SDL/include
else
    detected_OS := $(shell uname)  # same as "uname -s"

    CC := cc
    MAKE := make
    CFLAGS := -O2 -g -I SDL/include

    ifeq ($(detected_OS),Darwin)
	LDFLAGS := -framework Cocoa -framework IOKit -framework CoreFoundation
    else
    	LDFLAGS := -lm
    endif
endif


EXE := MCoder

SRC := $(shell find . -maxdepth 1 -name "*.c")
OBJ := $(SRC:.c=.o)

$(EXE): $(OBJ) build/libSDL2.a
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $< -c -o $@ $(CFLAGS)

build/libSDL2.a: SDL/src
	cmake -DSDL_STATIC=ON -DSDL_SHARED=OFF -S SDL -B build
	cmake --build build

SDL/src:
	git submodule update --init --recursive

.PHONY: clean build run

build: $(EXE)

run: $(EXE)
	./$(EXE)

clean:
	rm -f $(EXE)
