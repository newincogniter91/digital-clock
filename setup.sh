#!/usr/bin/env bash
# ══════════════════════════════════════════════════════════
#  setup.sh — DigitalClock
#  Rileva distro + architettura, chiede cosa compilare,
#  installa i pacchetti necessari e lancia la build.
#
#  NOTA ONESTA sul cross-compiling ARM<->x86:
#  Su Debian/Ubuntu è automatizzabile: dpkg supporta pacchetti
#  "multiarch" (es. libsdl2-dev:arm64) via il mirror ufficiale
#  ports.ubuntu.com per le architetture ARM. Questo script lo fa
#  in automatico su questa famiglia di distro.
#  Su Arch/Fedora/openSUSE NON esiste un sistema equivalente
#  pronto per librerie come SDL2/curl cross-compilate: pacman/dnf/
#  zypper non hanno un multiarch generico paragonabile a dpkg.
#  Su queste distro lo script installa solo la build nativa in
#  automatico, e per l'ARM ti spiega le alternative (compilare
#  direttamente sul dispositivo ARM è la via pratica).
# ══════════════════════════════════════════════════════════

set -e

echo "═══════════════════════════════════════════════"
echo "  DigitalClock — setup compilazione"
echo "═══════════════════════════════════════════════"
echo

# ── Rilevamento distro ──────────────────────────────────────
if [ ! -f /etc/os-release ]; then
    echo "Impossibile rilevare la distribuzione (/etc/os-release assente)."
    exit 1
fi
. /etc/os-release
ID_LOWER=$(echo "${ID_LIKE:-} ${ID:-}" | tr '[:upper:]' '[:lower:]')

FAMILY=""
case "$ID_LOWER" in
    *debian*|*ubuntu*) FAMILY="debian" ;;
    *arch*)             FAMILY="arch" ;;
    *fedora*|*rhel*)    FAMILY="fedora" ;;
    *suse*)              FAMILY="suse" ;;
esac

# ── Rilevamento architettura host ───────────────────────────
HOSTARCH=$(uname -m)
case "$HOSTARCH" in
    x86_64)          HOST_KIND="x86" ;;
    aarch64)          HOST_KIND="arm64" ;;
    armv7l|armv6l)   HOST_KIND="arm32" ;;
    i686|i386)        HOST_KIND="x86" ;;
    *)                 HOST_KIND="sconosciuta" ;;
esac

echo "Distribuzione : ${NAME:-sconosciuta}  (famiglia: ${FAMILY:-non riconosciuta})"
echo "Architettura  : $HOSTARCH  ($HOST_KIND)"
echo

if [ -z "$FAMILY" ]; then
    echo "Famiglia distro non riconosciuta — questo script sa automatizzare"
    echo "solo Debian/Ubuntu, Arch, Fedora/RHEL, openSUSE."
    echo "Installa manualmente: gcc, make, SDL2 dev headers, libcurl dev headers."
    exit 1
fi

# ── Menu interattivo ─────────────────────────────────────────
echo "Cosa vuoi compilare?"
echo "  1) Solo nativo per questa macchina ($HOST_KIND)"
echo "  2) x86 64-bit"
echo "  3) x86 32-bit"
echo "  4) ARM 64-bit (aarch64)"
echo "  5) ARM 32-bit (armhf)"
echo "  6) Tutto (x86 64/32 + ARM 64/32)"
echo
read -rp "Scelta [1-6]: " CHOICE
echo

WANT_X64=0; WANT_X32=0; WANT_ARM64=0; WANT_ARM32=0
case "$CHOICE" in
    1)
        case "$HOST_KIND" in
            x86)   WANT_X64=1 ;;
            arm64) WANT_ARM64=1 ;;
            arm32) WANT_ARM32=1 ;;
        esac
        ;;
    2) WANT_X64=1 ;;
    3) WANT_X32=1 ;;
    4) WANT_ARM64=1 ;;
    5) WANT_ARM32=1 ;;
    6) WANT_X64=1; WANT_X32=1; WANT_ARM64=1; WANT_ARM32=1 ;;
    *) echo "Scelta non valida."; exit 1 ;;
esac

# ── Debian/Ubuntu: installazione automatica reale ───────────
if [ "$FAMILY" = "debian" ]; then
    echo "Famiglia Debian-based rilevata — installazione automatica."
    echo "Verranno eseguiti comandi con sudo, ti verrà chiesta la password."
    echo

    PKGS="build-essential libsdl2-dev libcurl4-openssl-dev"
    sudo apt update
    sudo apt install -y $PKGS

    if [ "$WANT_X32" = "1" ] && [ "$HOST_KIND" = "x86" ]; then
        echo
        echo "--- Abilito supporto 32-bit (i386) ---"
        sudo dpkg --add-architecture i386
        sudo apt update
        sudo apt install -y gcc-multilib libsdl2-dev:i386 libcurl4-openssl-dev:i386
    fi

    if { [ "$WANT_ARM64" = "1" ] || [ "$WANT_ARM32" = "1" ]; } && [ "$HOST_KIND" = "x86" ]; then
        echo
        echo "--- Abilito cross-compiling verso ARM (richiede ports.ubuntu.com) ---"
        CODENAME="${VERSION_CODENAME:-noble}"
        sudo tee /etc/apt/sources.list.d/ubuntu-ports.sources > /dev/null << EOF
Types: deb
URIs: http://ports.ubuntu.com/ubuntu-ports
Suites: ${CODENAME} ${CODENAME}-updates ${CODENAME}-security
Components: main universe
Architectures: arm64 armhf
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF
        [ "$WANT_ARM64" = "1" ] && sudo dpkg --add-architecture arm64
        [ "$WANT_ARM32" = "1" ] && sudo dpkg --add-architecture armhf
        sudo apt update

        if [ "$WANT_ARM64" = "1" ]; then
            sudo apt install -y gcc-aarch64-linux-gnu libsdl2-dev:arm64 libcurl4-openssl-dev:arm64
        fi
        if [ "$WANT_ARM32" = "1" ]; then
            sudo apt install -y gcc-arm-linux-gnueabihf libsdl2-dev:armhf libcurl4-openssl-dev:armhf
        fi
    fi

    if [ "$WANT_X64" = "1" ] && { [ "$HOST_KIND" = "arm64" ] || [ "$HOST_KIND" = "arm32" ]; }; then
        echo
        echo "--- Abilito cross-compiling verso x86_64 (host è ARM) ---"
        sudo dpkg --add-architecture amd64
        sudo apt update
        sudo apt install -y gcc-x86-64-linux-gnu libsdl2-dev:amd64 libcurl4-openssl-dev:amd64
    fi

    echo
    echo "═══════════════════════════════════════════════"
    echo "  Compilazione"
    echo "═══════════════════════════════════════════════"

    [ "$WANT_X64" = "1" ]   && [ "$HOST_KIND" = "x86" ]   && make linux64
    [ "$WANT_X32" = "1" ]   && [ "$HOST_KIND" = "x86" ]   && make linux32
    [ "$WANT_ARM64" = "1" ] && [ "$HOST_KIND" = "arm64" ] && make linux-arm64
    [ "$WANT_ARM32" = "1" ] && [ "$HOST_KIND" = "arm32" ] && make linux-arm32
    [ "$WANT_ARM64" = "1" ] && [ "$HOST_KIND" = "x86" ]   && make cross-arm64
    [ "$WANT_ARM32" = "1" ] && [ "$HOST_KIND" = "x86" ]   && make cross-armhf
    [ "$WANT_X64" = "1" ]   && [ "$HOST_KIND" != "x86" ]  && make cross-x86_64

    echo
    echo "Fatto. Controlla la cartella corrente per gli eseguibili."
    exit 0
fi

# ── Arch / Fedora / openSUSE: solo build nativa automatica ──
echo "Famiglia $FAMILY rilevata."
echo "Questo script installa in automatico SOLO la build nativa"
echo "per l'architettura di questa macchina ($HOST_KIND)."
echo "Il cross-compiling verso ARM non è automatizzabile in modo"
echo "affidabile su questa famiglia di distro (pacman/dnf/zypper"
echo "non hanno un sistema multiarch per librerie come dpkg)."
echo "Per una build ARM: compila direttamente sul dispositivo ARM"
echo "(es. Raspberry Pi) con 'make linux-arm64' o 'make linux-arm32'."
echo

case "$FAMILY" in
    arch)
        sudo pacman -S --needed base-devel sdl2 curl
        if [ "$WANT_X32" = "1" ] && [ "$HOST_KIND" = "x86" ]; then
            echo "Per il 32-bit abilita il repo [multilib] in /etc/pacman.conf, poi:"
            echo "  sudo pacman -S lib32-sdl2 lib32-curl lib32-gcc-libs"
        fi
        ;;
    fedora)
        sudo dnf install -y gcc make SDL2-devel libcurl-devel
        ;;
    suse)
        sudo zypper install -y gcc make libSDL2-devel libcurl-devel
        ;;
esac

echo
echo "═══════════════════════════════════════════════"
echo "  Compilazione nativa"
echo "═══════════════════════════════════════════════"
make native

echo
echo "Fatto."

