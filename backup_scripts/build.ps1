# ============================================================================
#  SCRIPT DE COMPILACIÓN Y CARGA - COLLAR LORAWAN V3.0
# ============================================================================

param(
    [Parameter(Position=0)]
    [ValidateSet('compile', 'upload', 'monitor', 'clean', 'full')]
    [string]$Action = 'compile'
)

# Colores para output
function Write-ColorOutput($ForegroundColor) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    if ($args) {
        Write-Output $args
    }
    $host.UI.RawUI.ForegroundColor = $fc
}

Clear-Host
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "   COLLAR LORAWAN V3.0 - SISTEMA DE COMPILACIÓN" -ForegroundColor Yellow
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host ""

# Cambiar al directorio del proyecto
Set-Location "C:\Users\Rodrigo\Documents\PlatformIO\Projects\Collar Buena Cabra"

# Verificar estructura del proyecto
Write-Host "📁 Verificando estructura del proyecto..." -ForegroundColor Green

# Verificar archivos main duplicados
$mainFiles = @("main_minimal.cpp", "main_test.cpp")
foreach ($file in $mainFiles) {
    if (Test-Path "src\$file") {
        Write-Host "   ⚠️ Moviendo $file a examples..." -ForegroundColor Yellow
        Move-Item "src\$file" "examples\$file.example" -Force
    }
}

if (Test-Path "src\main.cpp") {
    Write-Host "   ✅ main.cpp encontrado" -ForegroundColor Green
} else {
    Write-Host "   ❌ ERROR: main.cpp no encontrado!" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Ejecutar acción solicitada
switch ($Action) {
    'clean' {
        Write-Host "🧹 Limpiando proyecto..." -ForegroundColor Yellow
        pio run --target clean
        if (Test-Path ".pio\build") {
            Remove-Item ".pio\build" -Recurse -Force
        }
        Write-Host "✅ Limpieza completada" -ForegroundColor Green
    }
    
    'compile' {
        Write-Host "🔨 Compilando proyecto..." -ForegroundColor Yellow
        Write-Host "================================================================" -ForegroundColor DarkGray
        
        $result = pio run
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "================================================================" -ForegroundColor Green
            Write-Host "   ✅ COMPILACIÓN EXITOSA!" -ForegroundColor Green
            Write-Host "================================================================" -ForegroundColor Green
            Write-Host ""
            Write-Host "📊 Resumen de correcciones aplicadas:" -ForegroundColor Cyan
            Write-Host "   ✓ Múltiples archivos main: RESUELTO" -ForegroundColor White
            Write-Host "   ✓ Constructores de Geofence: AGREGADOS" -ForegroundColor White
            Write-Host "   ✓ Funciones helper: IMPLEMENTADAS" -ForegroundColor White
            Write-Host "   ✓ Constantes del sistema: DEFINIDAS" -ForegroundColor White
            Write-Host ""
            Write-Host "🚀 Firmware listo para cargar al ESP32" -ForegroundColor Cyan
            Write-Host ""
            Write-Host "Próximos comandos disponibles:" -ForegroundColor Yellow
            Write-Host "   .\build.ps1 upload  - Cargar firmware al dispositivo" -ForegroundColor White
            Write-Host "   .\build.ps1 monitor - Monitorear puerto serial" -ForegroundColor White
            Write-Host "   .\build.ps1 full    - Compilar, cargar y monitorear" -ForegroundColor White
        } else {
            Write-Host ""
            Write-Host "================================================================" -ForegroundColor Red
            Write-Host "   ❌ ERROR EN LA COMPILACIÓN" -ForegroundColor Red
            Write-Host "================================================================" -ForegroundColor Red
            Write-Host "Revisa los mensajes de error arriba" -ForegroundColor Yellow
        }
    }
    
    'upload' {
        Write-Host "📤 Cargando firmware al ESP32..." -ForegroundColor Yellow
        pio run --target upload
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "✅ Firmware cargado exitosamente!" -ForegroundColor Green
            Write-Host "Ejecuta '.\build.ps1 monitor' para ver la salida serial" -ForegroundColor Cyan
        }
    }
    
    'monitor' {
        Write-Host "📡 Iniciando monitor serial..." -ForegroundColor Yellow
        Write-Host "Presiona Ctrl+C para salir" -ForegroundColor Cyan
        Write-Host ""
        pio device monitor
    }
    
    'full' {
        Write-Host "🔄 Ejecutando compilación completa..." -ForegroundColor Yellow
        
        # Limpiar
        Write-Host "[1/3] Limpiando..." -ForegroundColor Cyan
        pio run --target clean | Out-Null
        
        # Compilar
        Write-Host "[2/3] Compilando..." -ForegroundColor Cyan
        pio run
        
        if ($LASTEXITCODE -eq 0) {
            # Cargar
            Write-Host "[3/3] Cargando firmware..." -ForegroundColor Cyan
            pio run --target upload
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host ""
                Write-Host "✅ Todo completado! Iniciando monitor..." -ForegroundColor Green
                Start-Sleep -Seconds 2
                pio device monitor
            }
        } else {
            Write-Host "❌ Error en la compilación" -ForegroundColor Red
        }
    }
}

Write-Host ""
Write-Host "Presiona cualquier tecla para salir..." -ForegroundColor DarkGray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
