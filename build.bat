@echo off
echo ========================================
echo COLLAR LORAWAN V3.0 - BUILD SCRIPT
echo ========================================
echo 📱 Hardware: Heltec WiFi LoRa 32 V3
echo 🧠 MCU: ESP32-S3FN8
echo 📡 Radio: SX1262 + RadioLib
echo 🌍 Region: AU915 (Chile)
echo ========================================
echo.

echo 🔍 Verificando entorno PlatformIO...
call pio --version
if %ERRORLEVEL% NEQ 0 (
    echo ❌ ERROR: PlatformIO no encontrado
    echo Instalar con: pip install platformio
    pause
    exit /b 1
)

echo.
echo 🧹 Limpiando proyecto anterior...
call pio run --target clean
if %ERRORLEVEL% NEQ 0 (
    echo ⚠️  Warning: Limpieza falló, continuando...
)

echo.
echo 📚 Verificando dependencias...
call pio pkg list
echo.

echo 🔨 Compilando firmware V3.0...
echo ⏰ Esto puede tardar 1-2 minutos en primera compilación...
call pio run

echo.
if %ERRORLEVEL% EQU 0 (
    echo ✅ ¡COMPILACIÓN EXITOSA!
    echo.
    echo 📊 Información del firmware:
    call pio run --target size
    echo.
    echo 🎯 Próximos pasos:
    echo    1. Conectar Heltec V3 via USB-C
    echo    2. Ejecutar: upload.bat
    echo    3. Abrir monitor serie: pio device monitor
    echo.
    echo 📡 ¿Cargar firmware ahora? (s/n)
    set /p choice="Respuesta: "
    if /i "%choice%"=="s" (
        echo.
        echo 🚀 Cargando firmware...
        call pio run --target upload
        if %ERRORLEVEL% EQU 0 (
            echo.
            echo ✅ ¡FIRMWARE CARGADO EXITOSAMENTE!
            echo.
            echo 🖥️  ¿Abrir monitor serie? (s/n)
            set /p monitor="Respuesta: "
            if /i "%monitor%"=="s" (
                echo.
                echo 📺 Abriendo monitor serie (115200 baud)...
                echo Presiona Ctrl+C para salir
                call pio device monitor --baud 115200
            )
        ) else (
            echo ❌ ERROR CARGANDO FIRMWARE
            echo Verificar conexión USB y puerto COM
        )
    )
) else (
    echo ❌ ERROR DE COMPILACIÓN
    echo.
    echo 🔧 Posibles soluciones:
    echo    1. Verificar que todas las librerías estén instaladas
    echo    2. Comprobar sintaxis del código
    echo    3. Limpiar cache: pio run --target clean
    echo    4. Reinstalar dependencias: pio pkg install
    echo.
    echo 📋 Log de error guardado en .pio/build.log
)

echo.
echo ========================================
pause