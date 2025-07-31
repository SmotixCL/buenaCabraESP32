@echo off
echo ========================================
echo COLLAR LORAWAN V3.0 - CONFIGURADOR
echo ========================================
echo 🐐 Configuración rápida por tipo de animal
echo 🎵 Personalización de alertas
echo 📡 Setup LoRaWAN
echo ========================================
echo.

echo 🐾 SELECCIONA EL PERFIL DE ANIMAL:
echo.
echo    1. 🐐 CABRAS (actual)
echo       - Radio alerta: 20m
echo       - Sensibilidad: Alta
echo       - Volumen: Moderado
echo.
echo    2. 🐄 GANADO BOVINO
echo       - Radio alerta: 40m
echo       - Sensibilidad: Baja
echo       - Volumen: Alto
echo.
echo    3. 🐑 OVEJAS
echo       - Radio alerta: 25m
echo       - Sensibilidad: Media
echo       - Volumen: Moderado
echo.
echo    4. 🐕 MASCOTAS (Ultra sensible)
echo       - Radio alerta: 10m
echo       - Sensibilidad: Muy alta
echo       - Volumen: Bajo
echo.
echo    5. 🌙 MODO NOCTURNO
echo       - Radio alerta: 5m
echo       - Solo alertas críticas
echo       - Volumen: Muy bajo
echo.
echo    6. ⚙️  CONFIGURACIÓN PERSONALIZADA
echo.
echo    7. 🔧 CONFIGURAR CLAVES LORAWAN
echo.

set /p profile="Selecciona perfil (1-7): "

if "%profile%"=="1" (
    echo.
    echo 🐐 Configurando perfil CABRAS...
    call :configure_goats
) else if "%profile%"=="2" (
    echo.
    echo 🐄 Configurando perfil GANADO BOVINO...
    call :configure_cattle
) else if "%profile%"=="3" (
    echo.
    echo 🐑 Configurando perfil OVEJAS...
    call :configure_sheep
) else if "%profile%"=="4" (
    echo.
    echo 🐕 Configurando perfil MASCOTAS...
    call :configure_pets
) else if "%profile%"=="5" (
    echo.
    echo 🌙 Configurando MODO NOCTURNO...
    call :configure_night
) else if "%profile%"=="6" (
    echo.
    echo ⚙️  Configuración personalizada...
    call :configure_custom
) else if "%profile%"=="7" (
    echo.
    echo 🔧 Configurando claves LoRaWAN...
    call :configure_lorawan
) else (
    echo.
    echo ❌ Opción inválida
    pause
    exit /b 1
)

echo.
echo ✅ Configuración completada
echo.
echo 🔨 ¿Compilar y cargar ahora? (s/n)
set /p compile="Respuesta: "

if /i "%compile%"=="s" (
    echo.
    call build.bat
)

pause
exit /b 0

:configure_goats
echo Aplicando configuración para CABRAS...
powershell -Command "(gc src\config_rapida.h) -replace '#define GEOFENCE_ALERT_RADIUS.*', '#define GEOFENCE_ALERT_RADIUS   20.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define SAFE_DISTANCE.*', '#define SAFE_DISTANCE          15.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define CAUTION_DISTANCE.*', '#define CAUTION_DISTANCE       10.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define WARNING_DISTANCE.*', '#define WARNING_DISTANCE        5.0' | Out-File -encoding UTF8 src\config_rapida.h"
echo ✅ Perfil CABRAS configurado
goto :eof

:configure_cattle
echo Aplicando configuración para GANADO BOVINO...
powershell -Command "(gc src\config_rapida.h) -replace '#define GEOFENCE_ALERT_RADIUS.*', '#define GEOFENCE_ALERT_RADIUS   40.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define SAFE_DISTANCE.*', '#define SAFE_DISTANCE          30.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define CAUTION_DISTANCE.*', '#define CAUTION_DISTANCE       20.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define WARNING_DISTANCE.*', '#define WARNING_DISTANCE       10.0' | Out-File -encoding UTF8 src\config_rapida.h"
echo ✅ Perfil GANADO BOVINO configurado
goto :eof

:configure_sheep
echo Aplicando configuración para OVEJAS...
powershell -Command "(gc src\config_rapida.h) -replace '#define GEOFENCE_ALERT_RADIUS.*', '#define GEOFENCE_ALERT_RADIUS   25.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define SAFE_DISTANCE.*', '#define SAFE_DISTANCE          18.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define CAUTION_DISTANCE.*', '#define CAUTION_DISTANCE       12.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define WARNING_DISTANCE.*', '#define WARNING_DISTANCE        6.0' | Out-File -encoding UTF8 src\config_rapida.h"
echo ✅ Perfil OVEJAS configurado
goto :eof

:configure_pets
echo Aplicando configuración para MASCOTAS...
powershell -Command "(gc src\config_rapida.h) -replace '#define GEOFENCE_ALERT_RADIUS.*', '#define GEOFENCE_ALERT_RADIUS   10.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define SAFE_DISTANCE.*', '#define SAFE_DISTANCE           8.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define CAUTION_DISTANCE.*', '#define CAUTION_DISTANCE        6.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define WARNING_DISTANCE.*', '#define WARNING_DISTANCE        3.0' | Out-File -encoding UTF8 src\config_rapida.h"
echo ✅ Perfil MASCOTAS configurado
goto :eof

:configure_night
echo Aplicando configuración MODO NOCTURNO...
powershell -Command "(gc src\config_rapida.h) -replace '#define GEOFENCE_ALERT_RADIUS.*', '#define GEOFENCE_ALERT_RADIUS    5.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define SAFE_DISTANCE.*', '#define SAFE_DISTANCE           4.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define CAUTION_DISTANCE.*', '#define CAUTION_DISTANCE        3.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define WARNING_DISTANCE.*', '#define WARNING_DISTANCE        1.0' | Out-File -encoding UTF8 src\config_rapida.h"
echo ✅ Perfil MODO NOCTURNO configurado
goto :eof

:configure_custom
echo.
echo ⚙️  CONFIGURACIÓN PERSONALIZADA
echo.
set /p alert_radius="Radio de alerta (metros, ej: 20): "
set /p safe_dist="Distancia segura (metros, ej: 15): "
set /p caution_dist="Distancia precaución (metros, ej: 10): "
set /p warning_dist="Distancia advertencia (metros, ej: 5): "

echo.
echo Aplicando configuración personalizada...
powershell -Command "(gc src\config_rapida.h) -replace '#define GEOFENCE_ALERT_RADIUS.*', '#define GEOFENCE_ALERT_RADIUS   %alert_radius%.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define SAFE_DISTANCE.*', '#define SAFE_DISTANCE          %safe_dist%.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define CAUTION_DISTANCE.*', '#define CAUTION_DISTANCE       %caution_dist%.0' | Out-File -encoding UTF8 src\config_rapida.h"
powershell -Command "(gc src\config_rapida.h) -replace '#define WARNING_DISTANCE.*', '#define WARNING_DISTANCE        %warning_dist%.0' | Out-File -encoding UTF8 src\config_rapida.h"
echo ✅ Configuración personalizada aplicada
goto :eof

:configure_lorawan
echo.
echo 🔧 CONFIGURACIÓN DE CLAVES LORAWAN
echo.
echo ⚠️  IMPORTANTE: Cada collar debe tener claves únicas
echo.
echo 📝 Abriendo archivo de claves para edición...
echo    Archivo: src\lorawan_keys.h
echo.
echo 🔑 Configurar las siguientes claves:
echo    - joinEUI (del proveedor LoRaWAN)
echo    - devEUI (único por collar)
echo    - appKey (secreta, única por collar)
echo.
echo ⏰ Presiona cualquier tecla para abrir el archivo...
pause >nul

notepad src\lorawan_keys.h

echo.
echo ✅ Archivo de claves abierto para edición
echo 💾 Recuerda guardar los cambios
echo.
echo 📋 CHECKLIST:
echo    ☐ joinEUI configurado
echo    ☐ devEUI único por collar  
echo    ☐ appKey generado y copiado
echo    ☐ Archivo guardado
echo.
goto :eof