# ──────────────────────────────────────────────────────────
#  Makefile — DigitalClock
#  Supporta: Windows (MinGW/MSYS2) e Linux cross-compile
# ──────────────────────────────────────────────────────────

FLAGS   = -mwindows -municode -O2
LIBS    = -lwinhttp -lws2_32
SRC     = clock.c

# ── Windows (MSYS2 MinGW 64-bit) ──────────────────────────
win64:
	gcc $(FLAGS) $(SRC) -o clock64.exe $(LIBS)

win32:
	gcc $(FLAGS) $(SRC) -o clock32.exe $(LIBS)

# ── Linux cross-compile → Windows ─────────────────────────
# sudo apt install mingw-w64
linux64:
	x86_64-w64-mingw32-gcc $(FLAGS) $(SRC) -o clock64.exe $(LIBS)

linux32:
	i686-w64-mingw32-gcc $(FLAGS) $(SRC) -o clock32.exe $(LIBS)

# ── Entrambi da Linux ──────────────────────────────────────
all: linux64 linux32

clean:
	rm -f clock64.exe clock32.exe
