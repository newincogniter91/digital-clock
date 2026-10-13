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
PKG_CONFIG ?= pkg-config

# Usa pkg-config come sorgente principale per SDL2 e lascia un fallback
# a sdl2-config per i sistemi legacy. La build 32-bit e quella ARM
# richiede però un ambiente PKG_CONFIG_LIBDIR/PKG_CONFIG_PATH corretto,
# perché le librerie SDL2 di architettura diversa non vanno mai lette
# dalle impostazioni native del host.
SDL2_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2 2>/dev/null || sdl2-config --cflags 2>/dev/null)
SDL2_LIBS   := $(shell $(PKG_CONFIG) --libs sdl2 2>/dev/null || sdl2-config --libs 2>/dev/null)

SDL2_I386_CFLAGS := $(shell PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig:/usr/lib32/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig:/usr/lib32/pkgconfig $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL2_I386_LIBS   := $(shell PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig:/usr/lib32/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig:/usr/lib32/pkgconfig $(PKG_CONFIG) --libs sdl2 2>/dev/null)

SDL2_ARM64_CFLAGS := $(shell PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL2_ARM64_LIBS   := $(shell PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig $(PKG_CONFIG) --libs sdl2 2>/dev/null)

SDL2_ARMHF_CFLAGS := $(shell PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/arm-linux-gnueabihf/pkgconfig $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL2_ARMHF_LIBS   := $(shell PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/arm-linux-gnueabihf/pkgconfig $(PKG_CONFIG) --libs sdl2 2>/dev/null)

# ── Linux x86 nativo ─────────────────────────────────────────
# Richiede: gcc, libsdl2-dev, libcurl-dev (vedi setup.sh)
linux64:
	gcc -O2 -o clock_linux64 $(SRC) $(SDL2_CFLAGS) $(SDL2_LIBS) -lcurl -lm

linux32:
	gcc -O2 -m32 -o clock_linux32 $(SRC) $(SDL2_I386_CFLAGS) $(SDL2_I386_LIBS) -lcurl -lm

# ── Linux ARM nativo (Raspberry Pi, ecc.) ────────────────────
# Da lanciare DIRETTAMENTE sul dispositivo ARM.
linux-arm64:
	gcc -O2 -o clock_linux_arm64 $(SRC) $(SDL2_ARM64_CFLAGS) $(SDL2_ARM64_LIBS) -lcurl -lm

linux-arm32:
	gcc -O2 -o clock_linux_arm32 $(SRC) $(SDL2_ARMHF_CFLAGS) $(SDL2_ARMHF_LIBS) -lcurl -lm

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
	PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig \
	PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig \
	aarch64-linux-gnu-gcc -O2 -D_REENTRANT \
		$(shell PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig $(PKG_CONFIG) --cflags sdl2 2>/dev/null) \
		-o clock_linux_arm64 $(SRC) \
		$(shell PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig $(PKG_CONFIG) --libs sdl2 2>/dev/null) \
		-lcurl -lm

cross-armhf:
	PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig \
	PKG_CONFIG_LIBDIR=/usr/lib/arm-linux-gnueabihf/pkgconfig \
	arm-linux-gnueabihf-gcc -O2 -D_REENTRANT \
		$(shell PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/arm-linux-gnueabihf/pkgconfig $(PKG_CONFIG) --cflags sdl2 2>/dev/null) \
		-o clock_linux_arm32 $(SRC) \
		$(shell PKG_CONFIG_PATH=/usr/lib/arm-linux-gnueabihf/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/arm-linux-gnueabihf/pkgconfig $(PKG_CONFIG) --libs sdl2 2>/dev/null) \
		-lcurl -lm

# ── Cross-compile ARM -> x86 (richiede pacchetti multiarch, vedi setup.sh) ──
cross-x86_64:
	PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig \
	PKG_CONFIG_LIBDIR=/usr/lib/x86_64-linux-gnu/pkgconfig \
	x86_64-linux-gnu-gcc -O2 -D_REENTRANT \
		$(shell PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/x86_64-linux-gnu/pkgconfig $(PKG_CONFIG) --cflags sdl2 2>/dev/null) \
		-o clock_linux64 $(SRC) \
		$(shell PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig PKG_CONFIG_LIBDIR=/usr/lib/x86_64-linux-gnu/pkgconfig $(PKG_CONFIG) --libs sdl2 2>/dev/null) \
		-lcurl -lm

# ── Windows — da eseguire DENTRO una shell MSYS2 ───────────
# (MinGW64 shell per windows64, MinGW32 shell per windows32)
# Richiede: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 make
# Link statico di SDL2 + meteo via WinHTTP: l'exe non richiede nessuna DLL esterna.
SDL2_STATIC_LIBS := $(shell $(PKG_CONFIG) --static --libs sdl2 2>/dev/null)

windows64:
	gcc -O2 -mwindows -o clock64.exe $(SRC) $(SDL2_CFLAGS) -static $(SDL2_STATIC_LIBS) -lwinhttp -lm

windows32:
	gcc -O2 -mwindows -o clock32.exe $(SRC) $(SDL2_CFLAGS) -static $(SDL2_STATIC_LIBS) -lwinhttp -lm

all: linux64

clean:
	rm -f clock_linux64 clock_linux32 clock_linux_arm64 clock_linux_arm32 clock64.exe clock32.exe

