@echo off
echo ================================================
echo   COMPILACION PROYECTO COLLAR LORAWAN V3.0
echo ================================================
echo.

cd /d "C:\Users\Rodrigo\Documents\PlatformIO\Projects\Collar Buena Cabra"

echo Limpiando compilacion anterior...
pio run --target clean

echo.
echo Compilando proyecto...
pio run

echo.
echo ================================================
echo   COMPILACION FINALIZADA
echo ================================================
echo.
pause
