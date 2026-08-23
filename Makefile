# ──────────────────────────────────────────────────────────
#  Makefile — DigitalClock (SDL2, multi-OS)
#
#  I target Linux sono build NATIVE, non cross-compile: vanno
#  lanciati sulla stessa architettura del target.
#  Su un PC x86 usa linux64 / linux32.
#  Su Raspberry Pi o altro dispositivo ARM usa
#  linux-arm64 / linux-arm32 (stesso comando, gcc targetizza
#  già l'architettura della macchina su cui gira).
#  "make native" rileva l'architettura corrente e sceglie da solo.
# ──────────────────────────────────────────────────────────

SRC = clock.c
ARCH := $(shell uname -m)

SDL2_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL2_LIBS   := $(shell sdl2-config --libs 2>/dev/null)

# ── Linux x86 nativo ─────────────────────────────────────────
# Richiede: gcc, libsdl2-dev, libcurl-dev (vedi setup.sh)
linux64:
	gcc -O2 -o clock_linux64 $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lm

linux32:
	gcc -O2 -m32 -o clock_linux32 $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lm

# ── Linux ARM nativo (Raspberry Pi, ecc.) ────────────────────
# Da lanciare DIRETTAMENTE sul dispositivo ARM.
linux-arm64:
	gcc -O2 -o clock_linux_arm64 $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lm

linux-arm32:
	gcc -O2 -o clock_linux_arm32 $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lm

# ── Rileva l'architettura corrente e compila quella giusta ──
native:
ifeq ($(ARCH),x86_64)
	$(MAKE) linux64
else ifeq ($(ARCH),aarch64)
	$(MAKE) linux-arm64
else ifeq ($(ARCH),armv7l)
	$(MAKE) linux-arm32
else ifeq ($(ARCH),i686)
	$(MAKE) linux32
else
	@echo "Architettura non riconosciuta: $(ARCH)"
endif

# ── Cross-compile x86 -> ARM (richiede pacchetti multiarch, vedi setup.sh) ──
cross-arm64:
	aarch64-linux-gnu-gcc -O2 \
		-I/usr/include/aarch64-linux-gnu/SDL2 -D_REENTRANT \
		-o clock_linux_arm64 $(SRC) \
		-L/usr/lib/aarch64-linux-gnu -lSDL2 -lcurl -lm

cross-armhf:
	arm-linux-gnueabihf-gcc -O2 \
		-I/usr/include/arm-linux-gnueabihf/SDL2 -D_REENTRANT \
		-o clock_linux_arm32 $(SRC) \
		-L/usr/lib/arm-linux-gnueabihf -lSDL2 -lcurl -lm

# ── Cross-compile ARM -> x86 (richiede pacchetti multiarch, vedi setup.sh) ──
cross-x86_64:
	x86_64-linux-gnu-gcc -O2 \
		-I/usr/include/x86_64-linux-gnu/SDL2 -D_REENTRANT \
		-o clock_linux64 $(SRC) \
		-L/usr/lib/x86_64-linux-gnu -lSDL2 -lcurl -lm

# ── Windows — da eseguire DENTRO una shell MSYS2 ───────────
# (MinGW64 shell per windows64, MinGW32 shell per windows32)
# Richiede: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-curl
windows64:
	gcc -O2 -mwindows -o clock64.exe $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lws2_32 -lm

windows32:
	gcc -O2 -mwindows -o clock32.exe $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lws2_32 -lm

all: linux64

clean:
	rm -f clock_linux64 clock_linux32 clock_linux_arm64 clock_linux_arm32 clock64.exe clock32.exe

