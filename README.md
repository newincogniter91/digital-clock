# DigitalClock

Orologio digitale stile sveglia anni '80 per Windows — sfondo nero assoluto, cifre 7-segmenti bianche, meteo live.  
Applicazione nativa Win32, zero dipendenze esterne, zero browser, zero installer.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Funzionalità

| Elemento | Dettaglio |
|---|---|
| **Ora : Minuti** | Cifre 7-segmenti grandi, due punti lampeggianti ogni secondo |
| **Secondi** | Piccoli, in basso a destra dei minuti |
| **Data** | GG/MM/AA in alto al centro |
| **Giorno** | In italiano (LUNEDÌ, MARTEDÌ…) in alto a destra |
| **Meteo** | Icona + temperatura di Torino, aggiornata ogni 10 min via Open-Meteo |
| **Sveglia** | Imposta ora/minuti, suona con beep per 30 secondi |
| **12h / 24h** | Toggle formato orario |
| **Fullscreen** | F11 o doppio click |
| **Standby** | Impedisce lo standby del PC mentre è aperta |
| **Icona** | Generata in memoria, nessun file .ico esterno |

---

## Tasti

| Tasto | Azione |
|---|---|
| `H` | Mostra popup comandi |
| `T` | Toggle 12h / 24h |
| `A` | Imposta sveglia |
| `F11` | Schermo intero / finestra |
| Doppio click | Schermo intero / finestra |
| `ESC` | Ferma sveglia → esci da fullscreen → esci |

---

## Compilazione

### Su Windows (MSYS2)

Installa MSYS2 da [msys2.org](https://www.msys2.org/), apri **MSYS2 MinGW 64-bit** e:

```bash
# Prima volta — installa GCC
pacman -S mingw-w64-x86_64-gcc

# Compila
make win64

# Oppure senza Make
gcc -mwindows -municode clock.c -o clock64.exe -lwinhttp -lws2_32
```

### Su Linux (cross-compile per Windows)

```bash
# Prima volta — installa MinGW
sudo apt install mingw-w64

# 64-bit
make linux64

# 32-bit
make linux32

# Entrambi
make all
```

---

## Personalizzare la città

Modifica le coordinate in `FetchWeatherThread()`:

```c
L"/v1/forecast?latitude=45.0703&longitude=7.6869&current_weather=true&timezone=auto"
```

Trova le coordinate su [open-meteo.com](https://open-meteo.com/).

---

## Licenza

MIT
