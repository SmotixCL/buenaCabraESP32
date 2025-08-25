@echo off
echo ====================================
echo LIMPIANDO PROYECTO PARA GITHUB
echo ====================================

echo Eliminando archivos .bat innecesarios...
del /q build.bat 2>nul
del /q configure.bat 2>nul
del /q monitor.bat 2>nul
del /q upload.bat 2>nul

echo Eliminando documentacion duplicada...
del /q COMPILACION.md 2>nul
del /q LIMPIEZA.md 2>nul
del /q README_V3_SIMPLIFICADO.md 2>nul

echo Limpiando build artifacts...
rmdir /s /q .pio\build 2>nul

echo ====================================
echo PROYECTO LIMPIO Y LISTO PARA GITHUB
echo ====================================
echo.
echo Archivos restantes importantes:
echo - README.md (oficial)
echo - platformio.ini (configuracion)
echo - src/ (codigo fuente)
echo - debugging/ (archivos historicos)
echo.
echo Siguiente paso: git add . y git commit
pause
