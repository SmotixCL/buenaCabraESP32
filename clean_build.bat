@echo off
echo ========================================
echo Limpiando cache de compilacion...
echo ========================================

echo.
echo Eliminando directorio .pio\build...
if exist ".pio\build" (
    rmdir /s /q ".pio\build"
    echo Directorio .pio\build eliminado
) else (
    echo Directorio .pio\build no encontrado
)

echo.
echo Eliminando directorio .pio\libdeps...
if exist ".pio\libdeps" (
    rmdir /s /q ".pio\libdeps"
    echo Directorio .pio\libdeps eliminado
) else (
    echo Directorio .pio\libdeps no encontrado
)

echo.
echo ========================================
echo Cache limpiado exitosamente!
echo ========