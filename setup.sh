#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────
#  setup.sh — DigitalClock
#  Rileva la distro Linux e mostra quali pacchetti servono
#  e quale comando Makefile lanciare.
#  NON installa nulla in autonomia: mostra solo i comandi.
# ──────────────────────────────────────────────────────────

echo "═══════════════════════════════════════════════"
echo "  DigitalClock — controllo ambiente Linux"
echo "═══════════════════════════════════════════════"
echo

if [ ! -f /etc/os-release ]; then
    echo "Impossibile rilevare la distribuzione (/etc/os-release assente)."
    echo "Pacchetti generici richiesti: gcc, make, SDL2 dev headers, libcurl dev headers"
    exit 1
fi

. /etc/os-release
echo "Distribuzione rilevata: ${NAME:-sconosciuta}"
echo

ID_LOWER=$(echo "${ID_LIKE:-} ${ID:-}" | tr '[:upper:]' '[:lower:]')

FAMILY=""
case "$ID_LOWER" in
    *debian*|*ubuntu*) FAMILY="debian" ;;
    *arch*)             FAMILY="arch" ;;
    *fedora*|*rhel*)    FAMILY="fedora" ;;
    *suse*)              FAMILY="suse" ;;
esac

# ── Verifica cosa è già presente ────────────────────────────
echo "Verifica strumenti già installati:"
command -v gcc  >/dev/null 2>&1 && echo "  [OK]  gcc" || echo "  [--]  gcc mancante"
command -v make >/dev/null 2>&1 && echo "  [OK]  make" || echo "  [--]  make mancante"
if command -v pkg-config >/dev/null 2>&1; then
    pkg-config --exists sdl2    2>/dev/null && echo "  [OK]  SDL2 dev headers"    || echo "  [--]  SDL2 dev headers mancanti"
    pkg-config --exists libcurl 2>/dev/null && echo "  [OK]  libcurl dev headers" || echo "  [--]  libcurl dev headers mancanti"
else
    echo "  [??]  pkg-config non trovato, impossibile verificare SDL2/curl"
fi
echo

# ── Pacchetti per famiglia di distro ────────────────────────
case "$FAMILY" in
    debian)
        echo "Famiglia rilevata: Debian-based (apt) — Ubuntu, Debian, Mint, Pop!_OS..."
        echo
        echo "Da installare:"
        echo "  sudo apt update"
        echo "  sudo apt install build-essential libsdl2-dev libcurl4-openssl-dev"
        echo
        echo "Per compilare anche in 32-bit (richiede multilib):"
        echo "  sudo dpkg --add-architecture i386 && sudo apt update"
        echo "  sudo apt install gcc-multilib libsdl2-dev:i386 libcurl4-openssl-dev:i386"
        ;;
    arch)
        echo "Famiglia rilevata: Arch-based (pacman) — Arch, Manjaro, EndeavourOS..."
        echo
        echo "Da installare:"
        echo "  sudo pacman -S base-devel sdl2 curl"
        echo
        echo "Per il 32-bit serve il repository [multilib] abilitato, poi:"
        echo "  sudo pacman -S lib32-sdl2 lib32-curl lib32-gcc-libs"
        ;;
    fedora)
        echo "Famiglia rilevata: Fedora/RHEL-based (dnf) — Fedora, CentOS, Rocky, Alma..."
        echo
        echo "Da installare:"
        echo "  sudo dnf install gcc make SDL2-devel libcurl-devel"
        ;;
    suse)
        echo "Famiglia rilevata: openSUSE (zypper)"
        echo
        echo "Da installare:"
        echo "  sudo zypper install gcc make libSDL2-devel libcurl-devel"
        ;;
    *)
        echo "Distribuzione non riconosciuta automaticamente (ID: ${ID:-?})."
        echo "Cerca nel gestore pacchetti della tua distro:"
        echo "  - un compilatore C (gcc)"
        echo "  - make"
        echo "  - SDL2 development headers (es. libsdl2-dev / SDL2-devel / sdl2)"
        echo "  - libcurl development headers (es. libcurl-dev / libcurl-devel)"
        ;;
esac

echo
echo "───────────────────────────────────────────────"
echo "Questo script NON installa nulla — installa tu"
echo "i pacchetti indicati sopra, poi compila con:"
echo "───────────────────────────────────────────────"
echo

ARCH=$(uname -m)
if [ "$ARCH" = "x86_64" ]; then
    echo "  make linux64      # build a 64-bit (il tuo sistema è $ARCH)"
    echo "  make linux32      # build a 32-bit (richiede i pacchetti multilib sopra)"
else
    echo "  make linux32      # build a 32-bit (il tuo sistema è $ARCH)"
fi
echo
echo "L'eseguibile verrà creato nella cartella corrente."