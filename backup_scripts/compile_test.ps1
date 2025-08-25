# Script de compilación para el proyecto Collar Buena Cabra
Write-Host "================================================" -ForegroundColor Cyan
Write-Host "  COMPILACIÓN PROYECTO COLLAR LORAWAN V3.0" -ForegroundColor Yellow
Write-Host "================================================" -ForegroundColor Cyan
Write-Host ""

# Cambiar al directorio del proyecto
Set-Location "C:\Users\Rodrigo\Documents\PlatformIO\Projects\Collar Buena Cabra"

Write-Host "🔧 Limpiando compilación anterior..." -ForegroundColor Green
pio run --target clean

Write-Host ""
Write-Host "🔨 Compilando proyecto..." -ForegroundColor Green
pio run

# Verificar el resultado
if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "✅ COMPILACIÓN EXITOSA!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Resumen de correcciones aplicadas:" -ForegroundColor Yellow
    Write-Host "  ✓ Agregados constructores para struct Geofence" -ForegroundColor White
    Write-Host "  ✓ Agregada función isValidPosition()" -ForegroundColor White
    Write-Host "  ✓ Agregada función geofenceTypeToString()" -ForegroundColor White
    Write-Host "  ✓ Definidas constantes de distancia y batería" -ForegroundColor White
    Write-Host "  ✓ Corregida inicialización de thresholds" -ForegroundColor White
    Write-Host ""
    Write-Host "🚀 El proyecto está listo para cargar al ESP32!" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "❌ ERRORES EN LA COMPILACIÓN" -ForegroundColor Red
    Write-Host "Por favor revisa los mensajes de error arriba" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Presiona cualquier tecla para salir..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
