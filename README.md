# DigitalClock

A minimalist fullscreen digital clock for Windows, built with pure Win32 API and GDI.  
No browser, no Electron, no dependencies — just a single `.exe`.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

---

## What it does

- Displays current **time** (HH:MM) in large 7-segment style digits — like an old digital alarm clock
- **Seconds** shown small, bottom-right of the minutes
- **Date** in DD/MM/YY format, centered at the top
- **Day of the week** in Italian (LUNEDÌ, MARTEDÌ, ...), top-right
- **Live temperature** for Turin, Italy — top-left, fetched from [Open-Meteo](https://open-meteo.com/) (free API, no key required), updated every 10 minutes
- **Pure black background** (RGB 0,0,0) — looks great on OLED/AMOLED displays
- **Prevents standby** — screen and system stay awake while the app is running
- **Fullscreen support** — press F11 to toggle, ESC to exit

---

## Screenshots

> Classic 7-segment look, white digits on absolute black background.

---

## Requirements

- Windows 7 or later (x64)
- [MSYS2](https://www.msys2.org/) with MinGW64 toolchain (to compile)
- Internet connection (for temperature fetch)

---

## How to compile

### 1. Install MSYS2

Download and install from [msys2.org](https://www.msys2.org/).  
Open **"MSYS2 MinGW 64-bit"** from the Start menu.

### 2. Install GCC (first time only)

```bash
pacman -S mingw-w64-x86_64-gcc
```

### 3. Clone the repo and compile

```bash
cd /c/Users/YourName/Desktop
git clone https://github.com/youruser/DigitalClock
cd DigitalClock
gcc -mwindows -municode clock.c -o clock.exe -lwinhttp -lws2_32
```

That's it. No Makefile needed, no CMake, no Visual Studio.

---

## Usage

Just double-click `clock.exe` — no installer, no DLLs required.

| Key | Action |
|-----|--------|
| `F11` | Toggle fullscreen |
| `ESC` | Exit (or leave fullscreen) |

---

## How it works

| Feature | Implementation |
|---------|---------------|
| Window + rendering | Win32 API + GDI (pure C) |
| 7-segment digits | Drawn as polygons via `Polygon()` |
| Double buffering | Off-screen HDC to avoid flicker |
| Temperature fetch | `WinHTTP` → Open-Meteo REST API (HTTPS) |
| Standby prevention | `SetThreadExecutionState()` |
| Unicode / Italian | `wchar_t`, `DrawTextW()` |

---

## Customization

To use a different city, change the coordinates in `FetchWeatherThread()`:

```c
L"/v1/forecast?latitude=45.0703&longitude=7.6869"
//                         ^^^              ^^^^
//                      your lat         your lon
```

Find your coordinates on [open-meteo.com](https://open-meteo.com/).

---

## License

MIT — do whatever you want with it.
