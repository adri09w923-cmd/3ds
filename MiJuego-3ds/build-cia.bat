@echo off
set DKP=C:\devkitPro
if not exist "%DKP%\msys2\usr\bin\bash.exe" (echo No encuentro devkitPro en C:\devkitPro. Instalalo con devkitPro Updater y ejecuta: dkp-pacman -S 3ds-dev & pause & exit /b 1)
"%DKP%\msys2\usr\bin\bash.exe" -lc "cd \"$(cygpath -u '%~dp0')\" && make && bannertool makebanner -i cia/banner.png -a cia/silencio.wav -o cia/banner.bnr && bannertool makesmdh -s \"MiJuego\" -l \"Hecho con Motor de Bloques\" -p \"Motor de Bloques\" -i cia/icon.png -o cia/icon.icn && makerom -f cia -o juego.cia -elf MotorBloques3DS.elf -rsf cia/app.rsf -target t -exefslogo -icon cia/icon.icn -banner cia/banner.bnr && echo \"Listo: juego.cia (instálalo con FBI)\""
if errorlevel 1 (echo FALLO: revisa los errores de arriba. Falta 3ds-dev o bannertool/makerom? & pause & exit /b 1)
echo Listo: juego.cia
pause
