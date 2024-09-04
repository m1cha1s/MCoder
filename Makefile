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

    ifneq (,$(findstring Darwin,$(detected_OS)))
        LDFLAGS := -framework CoreVideo -framework Cocoa -framework IOKit -framework ForceFeedback -framework Carbon -framework CoreAudio \
        -framework AudioToolbox -framework AVFoundation -framework Foundation -weak_framework GameController -weak_framework Metal \
        -weak_framework QuartzCore -weak_framework CoreHaptics -lm
    else
        LDFLAGS := -lm
    endif
endif


EXE := MCoder

SRC := $(shell find . -maxdepth 1 -name "*.c")
OBJ := $(SRC:.c=.o)

$(EXE): $(OBJ) build/libSDL2.a
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c SDL/include
	$(CC) $< -c -o $@ $(CFLAGS)

build/libSDL2.a: SDL/include
	cmake -DSDL_STATIC=ON -DSDL_SHARED=OFF -S SDL -B build
	cmake --build build

SDL/include:
	git submodule update --init --recursive

.PHONY: clean build run

build: $(EXE)

run: $(EXE)
	./$(EXE)

clean:
	rm -f $(EXE)
