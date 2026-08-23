#!/usr/bin/env bash
# ══════════════════════════════════════════════════════════
#  package-linux.sh
#  Crea un pacchetto standalone per un eseguibile Linux:
#  trova tutte le librerie .so necessarie (dirette + transitive)
#  analizzando gli header ELF con objdump (senza eseguire il
#  binario — funziona anche per eseguibili cross-compilati per
#  un'altra architettura, tipo ARM compilato da un host x86_64),
#  le copia accanto al binario, e genera uno script di lancio
#  che imposta LD_LIBRARY_PATH.
#
#  Uso:
#    ./package-linux.sh <binario> <objdump-tool> <lib-dir1> [lib-dir2 ...]
#
#  Esempi:
#    ./package-linux.sh clock_linux64 objdump \
#        /usr/lib/x86_64-linux-gnu /lib/x86_64-linux-gnu
#    ./package-linux.sh clock_linux_arm64 aarch64-linux-gnu-objdump \
#        /usr/lib/aarch64-linux-gnu /lib/aarch64-linux-gnu
# ══════════════════════════════════════════════════════════

set -e

BINARY="$1"
OBJDUMP="$2"
shift 2
LIBDIRS=("$@")

if [ -z "$BINARY" ] || [ -z "$OBJDUMP" ] || [ ${#LIBDIRS[@]} -eq 0 ]; then
    echo "Uso: $0 <binario> <objdump-tool> <lib-dir1> [lib-dir2 ...]"
    exit 1
fi

if [ ! -f "$BINARY" ]; then
    echo "Errore: binario '$BINARY' non trovato."
    exit 1
fi

OUTDIR="dist-$(basename "$BINARY")"
LIBOUT="$OUTDIR/lib"
rm -rf "$OUTDIR"
mkdir -p "$LIBOUT"

# Librerie di base del sistema — NON vanno mai bundlate: sono
# legate all'ABI esatta di kernel/libc dell'utente finale, e
# bundlarle rischia di rompere l'app invece di aiutarla.
SKIP_LIBS="linux-vdso.so.1 ld-linux.so.2 ld-linux-x86-64.so.2 ld-linux-aarch64.so.1 ld-linux-armhf.so.3 libc.so.6 libm.so.6 libpthread.so.0 libdl.so.2 librt.so.1 libresolv.so.2 libgcc_s.so.1"

declare -A SEEN
QUEUE=("$BINARY")
COUNT=0

find_lib_path() {
    local name="$1"
    for d in "${LIBDIRS[@]}"; do
        if [ -f "$d/$name" ]; then
            echo "$d/$name"
            return 0
        fi
        # alcuni plugin (es. PulseAudio) vivono in una sottocartella
        if [ -f "$d/pulseaudio/$name" ]; then
            echo "$d/pulseaudio/$name"
            return 0
        fi
    done
    return 1
}

echo "Analisi dipendenze di $BINARY..."

while [ ${#QUEUE[@]} -gt 0 ]; do
    current="${QUEUE[0]}"
    QUEUE=("${QUEUE[@]:1}")

    needed=$("$OBJDUMP" -p "$current" 2>/dev/null | grep NEEDED | awk '{print $2}')
    for lib in $needed; do
        case " $SKIP_LIBS " in *" $lib "*) continue ;; esac
        [ -n "${SEEN[$lib]:-}" ] && continue
        SEEN[$lib]=1

        if libpath=$(find_lib_path "$lib"); then
            echo "  + $lib"
            cp -L "$libpath" "$LIBOUT/"
            QUEUE+=("$libpath")
            COUNT=$((COUNT+1))
        else
            echo "  ! $lib non trovata nelle lib-dir indicate, salto (potrebbe essere già di sistema)"
        fi
    done
done

cp "$BINARY" "$OUTDIR/"
BINNAME=$(basename "$BINARY")

cat > "$OUTDIR/run.sh" << LAUNCHER
#!/usr/bin/env bash
DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="\$DIR/lib:\$LD_LIBRARY_PATH"
exec "\$DIR/$BINNAME" "\$@"
LAUNCHER
chmod +x "$OUTDIR/run.sh" "$OUTDIR/$BINNAME"

echo
echo "Pacchetto creato in: $OUTDIR/"
echo "Librerie bundlate: $COUNT"
echo "Avvio con: ./$OUTDIR/run.sh"

