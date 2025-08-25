@echo off
cls
echo ================================================================
echo    COMPILACION LIMPIA - COLLAR LORAWAN V3.0
echo ================================================================
echo.

cd /d "C:\Users\Rodrigo\Documents\PlatformIO\Projects\Collar Buena Cabra"

echo [1/3] Limpiando archivos de compilacion anterior...
pio run --target clean > nul 2>&1
if exist .pio\build\heltec_wifi_lora_32_v3\*.o del /q .pio\build\heltec_wifi_lora_32_v3\*.o > nul 2>&1
if exist .pio\build\heltec_wifi_lora_32_v3\*.a del /q .pio\build\heltec_wifi_lora_32_v3\*.a > nul 2>&1
echo      OK - Limpieza completada

echo.
echo [2/3] Verificando estructura del proyecto...
if exist src\main.cpp (
    echo      OK - main.cpp encontrado
) else (
    echo      ERROR - main.cpp no encontrado!
    pause
    exit /b 1
)

if exist src\main_minimal.cpp (
    echo      ADVERTENCIA - main_minimal.cpp detectado, moviendo a examples...
    move src\main_minimal.cpp examples\main_minimal.cpp.example > nul 2>&1
)

if exist src\main_test.cpp (
    echo      ADVERTENCIA - main_test.cpp detectado, moviendo a examples...
    move src\main_test.cpp examples\main_test.cpp.example > nul 2>&1
)

echo.
echo [3/3] Compilando proyecto...
echo ----------------------------------------------------------------
pio run

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ================================================================
    echo    COMPILACION EXITOSA!
    echo ================================================================
    echo.
    echo Resumen:
    echo   - Archivos main duplicados: RESUELTO
    echo   - Errores de tipos: CORREGIDO
    echo   - Constantes faltantes: AGREGADAS
    echo   - Firmware listo para cargar
    echo.
    echo Siguientes pasos:
    echo   1. Conecta el ESP32 por USB
    echo   2. Ejecuta: pio run --target upload
    echo   3. Monitorea: pio device monitor
    echo.
) else (
    echo.
    echo ================================================================
    echo    ERROR EN LA COMPILACION
    echo ================================================================
    echo.
    echo Por favor revisa los mensajes de error arriba.
    echo.
)

pause
