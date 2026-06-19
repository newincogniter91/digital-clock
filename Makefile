# ──────────────────────────────────────────────────────────
#  Makefile per DigitalClock  (MinGW / MSYS2 su Windows)
# ──────────────────────────────────────────────────────────

CC      = gcc
TARGET  = clock.exe
SRC     = clock.c

# -mwindows   → no finestra console (solo GUI)
# -lwinhttp   → WinHTTP per fetch meteo
# -municode   → entry point wWinMain unicode
FLAGS   = -O2 -Wall -mwindows -municode
LIBS    = -lwinhttp -lws2_32

all:
	$(CC) $(FLAGS) $(SRC) -o $(TARGET) $(LIBS)

debug:
	$(CC) -O0 -g $(SRC) -o $(TARGET) -lwinhttp -lws2_32 -municode

clean:
	del /Q $(TARGET) 2>NUL || rm -f $(TARGET)


# ──────────────────────────────────────────────────────────
#  ISTRUZIONI RAPIDE
# ──────────────────────────────────────────────────────────
#
#  1) Installa MSYS2  →  https://www.msys2.org/
#     Apri "MSYS2 MinGW 64-bit" dal menu Start
#
#  2) Installa il compilatore (una volta sola):
#       pacman -S mingw-w64-x86_64-gcc
#
#  3) Compila:
#       cd /c/percorso/cartella
#       make
#     oppure senza Make:
#       gcc -O2 -mwindows -municode clock.c -o clock.exe -lwinhttp -lws2_32
#
#  4) Doppio clic su clock.exe — nessuna DLL aggiuntiva richiesta
#
#  TASTI:
#    F11      → schermo intero / finestra
#    ESC      → esci (o torna dalla modalità schermo intero)
#
#  METEO:
#    Richiede connessione Internet per la prima lettura.
#    Aggiornamento automatico ogni 10 minuti via Open-Meteo
#    (API pubblica gratuita, niente chiave necessaria).
#    Coordinate fisse: Torino  lat=45.07  lon=7.69
