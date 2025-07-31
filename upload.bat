@echo off
echo ========================================
echo COLLAR LORAWAN V3.0 - UPLOAD SCRIPT
echo ========================================
echo 📱 Hardware: Heltec WiFi LoRa 32 V3
echo 🔌 Conexión: USB-C
echo 🚀 Firmware: RadioLib + SX1262
echo ========================================
echo.

echo 🔍 Detectando dispositivos conectados...
call pio device list
echo.

echo 🔌 INSTRUCCIONES DE CONEXIÓN:
echo    1. Conectar Heltec V3 via cable USB-C
echo    2. Verificar que aparece en la lista de arriba
echo    3. Si no aparece, instalar drivers CH340/CP2102
echo.
echo 📱 ¿Está el collar conectado correctamente? (s/n)
set /p connected="Respuesta: "

if /i not "%connected%"=="s" (
    echo.
    echo 🛠️  AYUDA PARA CONEXIÓN:
    echo.
    echo Windows:
    echo    - Instalar drivers desde: https://www.wch.cn/downloads/
    echo    - Verificar en Administrador de dispositivos
    echo.
    echo ❌ Conexión cancelada por usuario
    pause
    exit /b 1
)

echo.
echo 🔨 Verificando que el firmware esté compilado...
if not exist ".pio\build\heltec_wifi_lora_32_v3\firmware.bin" (
    echo ❌ Firmware no encontrado
    echo 🔨 Compilando primero...
    call pio run
    if %ERRORLEVEL% NEQ 0 (
        echo ❌ Error en compilación
        pause
        exit /b 1
    )
) else (
    echo ✅ Firmware encontrado
)

echo.
echo 🚀 Cargando firmware V3.0 al collar...
echo ⏰ Esto tardará 10-30 segundos...
echo.
echo 💡 TIP: Si falla, presiona RST en el collar e intenta de nuevo
echo.

call pio run --target upload

echo.
if %ERRORLEVEL% EQU 0 (
    echo ✅ ¡FIRMWARE V3.0 CARGADO EXITOSAMENTE!
    echo.
    echo 🎯 QUÉ ESPERAR DESPUÉS DE LA CARGA:
    echo    ✅ LED parpadea 3 veces (test inicial)
    echo    🎵 Melodía de inicio (5 tonos ascendentes)
    echo    📺 OLED muestra "LoRaWAN Collar V3"
    echo    🧪 Test automático de buzzer (4 niveles)
    echo    📡 Configuración LoRaWAN AU915
    echo    🔗 Intento de JOIN automático
    echo.
    echo 📊 Información del firmware cargado:
    call pio run --target size
    echo.
    echo 🖥️  ¿Abrir monitor serie para ver logs? (s/n)
    set /p monitor="Respuesta: "
    if /i "%monitor%"=="s" (
        echo.
        echo 📺 Monitor serie iniciando (115200 baud)...
        echo 📌 Comandos útiles:
        echo    Ctrl+C = Salir del monitor
        echo    RST = Reiniciar collar (botón físico)
        echo.
        echo 🧪 TESTING RÁPIDO:
        echo    - Downlink 0x03 = Test completo buzzer
        echo    - Downlink 0x02 = Melodía nueva geocerca  
        echo    - Downlink 0x01 = Alerta manual
        echo.
        echo ⏳ Iniciando monitor en 3 segundos...
        timeout /t 3 /nobreak >nul
        call pio device monitor --baud 115200 --filter esp32_exception_decoder
    ) else (
        echo.
        echo 📱 Para monitorear posteriormente:
        echo    pio device monitor --baud 115200
        echo.
        echo 🎯 PRÓXIMOS PASOS RECOMENDADOS:
        echo    1. Configurar claves LoRaWAN únicas en main.cpp
        echo    2. Configurar gateway para AU915 sub-banda 2
        echo    3. Definir geocerca via downlink 0x02
        echo    4. Testing de campo a diferentes distancias
    )
) else (
    echo ❌ ERROR AL CARGAR FIRMWARE
    echo.
    echo 🔧 SOLUCIONES POSIBLES:
    echo    1. Verificar conexión USB-C bien conectada
    echo    2. Presionar y soltar botón RST en el collar
    echo    3. Probar otro puerto USB
    echo    4. Cerrar otros programas que usen puerto serie
    echo    5. Reinstalar drivers USB (CH340/CP2102)
    echo.
    echo 🔍 Para más información:
    echo    pio device list  (mostrar dispositivos)
    echo    pio run -v       (compilación verbose)
    echo.
    echo 📞 Si el problema persiste:
    echo    - Verificar que el cable USB-C sea de datos (no solo carga)
    echo    - Hardware puede estar defectuoso
)

echo.
echo ========================================
echo Presiona cualquier tecla para salir...
pause >nul