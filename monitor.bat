@echo off
echo ========================================
echo COLLAR LORAWAN V3.0 - MONITOR & DEBUG
echo ========================================
echo 📺 Monitor serie con decodificación avanzada
echo 🔍 Debug de excepciones ESP32-S3
echo 📊 Análisis en tiempo real
echo ========================================
echo.

echo 🔍 Detectando collar conectado...
call pio device list
echo.

echo 🔧 OPCIONES DE MONITOREO:
echo    1. Monitor básico (solo logs)
echo    2. Monitor con decodificador de excepciones
echo    3. Monitor con filtros de tiempo
echo    4. Monitor completo (todos los filtros)
echo    5. Diagnóstico completo del sistema
echo.

set /p option="Selecciona opción (1-5): "

if "%option%"=="1" (
    echo.
    echo 📺 Iniciando monitor básico...
    call pio device monitor --baud 115200
) else if "%option%"=="2" (
    echo.
    echo 📺 Iniciando monitor con decodificador de excepciones...
    call pio device monitor --baud 115200 --filter esp32_exception_decoder
) else if "%option%"=="3" (
    echo.
    echo 📺 Iniciando monitor con timestamp...
    call pio device monitor --baud 115200 --filter time
) else if "%option%"=="4" (
    echo.
    echo 📺 Iniciando monitor completo...
    call pio device monitor --baud 115200 --filter esp32_exception_decoder --filter time --filter colorize
) else if "%option%"=="5" (
    echo.
    echo 🔍 EJECUTANDO DIAGNÓSTICO COMPLETO...
    echo.
    
    echo 1️⃣ Información del proyecto:
    call pio project config
    echo.
    
    echo 2️⃣ Estado de librerías:
    call pio pkg list
    echo.
    
    echo 3️⃣ Información del board:
    call pio boards heltec_wifi_lora_32_V3
    echo.
    
    echo 4️⃣ Verificando compilación:
    call pio run --target size
    echo.
    
    echo 5️⃣ Dispositivos disponibles:
    call pio device list
    echo.
    
    echo 6️⃣ ¿Iniciar monitor después del diagnóstico? (s/n)
    set /p startmon="Respuesta: "
    if /i "%startmon%"=="s" (
        echo.
        echo 📺 Iniciando monitor completo...
        call pio device monitor --baud 115200 --filter esp32_exception_decoder --filter time
    )
) else (
    echo.
    echo ❌ Opción inválida
    pause
    exit /b 1
)

echo.
echo ========================================
echo Monitor finalizado
echo ========================================
pause