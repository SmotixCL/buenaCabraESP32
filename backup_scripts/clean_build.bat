@echo off
REM ============================================================================
REM Script de limpieza para Collar BuenaCabra
REM Elimina archivos temporales y de compilación
REM ============================================================================

echo.
echo ========================================
echo    LIMPIEZA DE PROYECTO PLATFORMIO
echo ========================================
echo.

REM Confirmar antes de limpiar
set /p confirm="Esto eliminara todos los archivos de compilacion. Continuar? (S/N): "
if /i "%confirm%" neq "S" goto :cancel

echo.
echo [1/5] Limpiando directorio .pio...
if exist .pio (
    rmdir /s /q .pio
    echo      - Eliminado .pio
) else (
    echo      - .pio no existe
)

echo.
echo [2/5] Limpiando archivos de compilacion...
if exist *.elf del /q *.elf && echo      - Eliminados archivos .elf
if exist *.bin del /q *.bin && echo      - Eliminados archivos .bin
if exist *.hex del /q *.hex && echo      - Eliminados archivos .hex

echo.
echo [3/5] Limpiando archivos temporales...
if exist *.tmp del /q *.tmp && echo      - Eliminados archivos .tmp
if exist *.temp del /q *.temp && echo      - Eliminados archivos .temp
if exist *.log del /q *.log && echo      - Eliminados archivos .log
if exist build_summary.txt del /q build_summary.txt && echo      - Eliminado build_summary.txt

echo.
echo [4/5] Limpiando cache de Python...
if exist __pycache__ (
    rmdir /s /q __pycache__
    echo      - Eliminado __pycache__
)
if exist scripts\__pycache__ (
    rmdir /s /q scripts\__pycache__
    echo      - Eliminado scripts\__pycache__
)

echo.
echo [5/5] Limpiando archivos de respaldo...
set /p backup="Eliminar archivos de respaldo (.bak, .backup)? (S/N): "
if /i "%backup%" equ "S" (
    if exist *.bak del /q *.bak && echo      - Eliminados archivos .bak
    if exist *.backup del /q *.backup && echo      - Eliminados archivos .backup
    if exist backup_original (
        set /p backupdir="Eliminar directorio backup_original? (S/N): "
        if /i "%backupdir%" equ "S" (
            rmdir /s /q backup_original
            echo      - Eliminado directorio backup_original
        )
    )
)

echo.
echo ========================================
echo    LIMPIEZA COMPLETADA
echo ========================================
echo.
echo Proyecto limpio y listo para compilar!
echo.
echo Siguientes pasos:
echo   1. pio run          - Para compilar
echo   2. pio run -t upload - Para cargar al dispositivo
echo   3. pio device monitor - Para ver el monitor serial
echo.
pause
goto :eof

:cancel
echo.
echo Limpieza cancelada.
echo.
pause
