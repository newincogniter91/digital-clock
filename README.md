# DigitalClock

Orologio digitale stile sveglia anni '80 — sfondo nero assoluto, cifre 7-segmenti, meteo live.  
Versione **multi-OS** (Windows + Linux) basata su SDL2, font bitmap incluso, nessun file esterno richiesto.

![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue)
![Language](https://img.shields.io/badge/language-C-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

> Cerchi la versione Windows-only senza dipendenze (Win32 puro)? È nel branch [`win32-native`](../../tree/win32-native).

---

## Funzionalità

| Elemento | Dettaglio |
|---|---|
| **Ora : Minuti** | Cifre 7-segmenti, due punti lampeggianti ogni secondo |
| **Secondi** | Piccoli, in basso a destra dei minuti |
| **Data** | GG/MM/AA in alto al centro |
| **Giorno** | In italiano (LUNEDI, MARTEDI...) in alto a destra |
| **Meteo** | Torino, aggiornato ogni 10 min via Open-Meteo, con etichetta condizione (SUN/CLOUD/RAIN/SNOW/STORM) |
| **Sveglia** | Overlay a schermo per impostare ora/minuti, beep generato al volo per 30s |
| **12h / 24h** | Toggle formato orario |
| **Fullscreen** | F11 o doppio click |
| **Standby** | Impedisce lo spegnimento schermo mentre è aperta |
| **Font** | Bitmap 5x7 autocostruito — nessun font di sistema richiesto |
| **Icona finestra** | Generata in memoria |

---

## Tasti

| Tasto | Azione |
|---|---|
| `H` | Mostra/chiudi popup comandi |
| `T` | Toggle 12h / 24h |
| `A` | Apri editor sveglia (TAB cambia campo, SU/GIÙ regola, SPAZIO on/off, INVIO conferma) |
| `F11` | Schermo intero / finestra |
| Doppio click | Schermo intero / finestra |
| `ESC` | Ferma sveglia → chiudi overlay → esci da fullscreen → esci |

---

## Compilazione

### Linux

Lo script `setup.sh` rileva la tua distro e mostra i pacchetti necessari (non installa nulla in autonomia):

```bash
chmod +x setup.sh
./setup.sh
```

Poi, a seconda dell'architettura:

```bash
make linux64     # 64-bit
make linux32     # 32-bit (richiede pacchetti multilib, vedi setup.sh)
```

**Dipendenze**: `gcc`, `make`, `libsdl2-dev` (o equivalente), `libcurl` dev headers.

### Windows (via MSYS2)

1. Installa [MSYS2](https://www.msys2.org/)
2. Apri **"MSYS2 MinGW 64-bit"** dal menu Start
3. Installa le dipendenze (una volta sola):
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 make
   ```
4. Compila:
   ```bash
   make windows64
   ```

Per la versione 32-bit, apri invece **"MSYS2 MinGW 32-bit"**, installa i pacchetti equivalenti con prefisso `mingw-w64-i686-` e lancia `make windows32`.

> Il cross-compile Windows→Linux direttamente da un sistema Linux non è supportato da questo Makefile: richiederebbe SDL2/curl compilati per target MinGW, non disponibili nei repository standard delle distro. La via supportata è MSYS2 nativo su Windows.

---

## Personalizzare la città

Modifica le coordinate in cima a `clock.c`:

```c
#define CITY_LAT "45.0703"
#define CITY_LON "7.6869"
```

Trova le tue coordinate su [open-meteo.com](https://open-meteo.com/).

---

## Licenza

MIT