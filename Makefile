# ──────────────────────────────────────────────────────────
#  Makefile — DigitalClock (SDL2, multi-OS)
# ──────────────────────────────────────────────────────────

SRC = clock.c

SDL2_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL2_LIBS   := $(shell sdl2-config --libs 2>/dev/null)

# ── Linux nativo ───────────────────────────────────────────
# Richiede: gcc, libsdl2-dev, libcurl-dev (vedi setup.sh)
linux64:
	gcc -O2 -o clock_linux64 $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lm

linux32:
	gcc -O2 -m32 -o clock_linux32 $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lm

# ── Windows — da eseguire DENTRO una shell MSYS2 ───────────
# (MinGW64 shell per windows64, MinGW32 shell per windows32)
# Richiede: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-curl
windows64:
	gcc -O2 -mwindows -o clock64.exe $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lws2_32 -lm

windows32:
	gcc -O2 -mwindows -o clock32.exe $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lws2_32 -lm

all: linux64

clean:
	rm -f clock_linux64 clock_linux32 clock64.exe clock32.exe