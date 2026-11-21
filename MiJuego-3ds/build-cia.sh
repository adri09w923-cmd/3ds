#!/bin/sh
# Genera juego.cia (necesita devkitPro + bannertool + makerom en el PATH)
set -e
cd "$(dirname "$0")"
make
bannertool makebanner -i cia/banner.png -a cia/silencio.wav -o cia/banner.bnr
bannertool makesmdh -s "MiJuego" -l "Hecho con Motor de Bloques" -p "Motor de Bloques" -i cia/icon.png -o cia/icon.icn
makerom -f cia -o juego.cia -elf MotorBloques3DS.elf -rsf cia/app.rsf -target t -exefslogo -icon cia/icon.icn -banner cia/banner.bnr
echo "Listo: juego.cia (instálalo con FBI)"
