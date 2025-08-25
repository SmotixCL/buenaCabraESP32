#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Post-build script for Collar BuenaCabra
Ejecuta después de compilar el proyecto
"""

Import("env", "projenv")
import os
from datetime import datetime

def after_build(source, target, env):
    """Función ejecutada después de la compilación"""
    
    print("\n" + "="*60)
    print("🐐 COLLAR BUENACABRA - POST-BUILD SCRIPT")
    print("="*60)
    
    # Obtener información del firmware compilado
    firmware_path = str(target[0])
    print(f"Firmware Path: {firmware_path}")
    
    if os.path.exists(firmware_path):
        size = os.path.getsize(firmware_path)
        print(f"Firmware Size: {size:,} bytes ({size/1024:.2f} KB)")
        
        # Calcular porcentaje de uso de flash (asumiendo 4MB)
        max_flash = 4 * 1024 * 1024  # 4MB
        percentage = (size / max_flash) * 100
        print(f"Flash Usage: {percentage:.2f}%")
        
        # Advertencias si el firmware es muy grande
        if percentage > 80:
            print("⚠️  WARNING: Flash usage is above 80%!")
        elif percentage > 60:
            print("⚠️  CAUTION: Flash usage is above 60%")
        else:
            print("✓ Flash usage is optimal")
    else:
        print("✗ Firmware file not found!")
    
    # Crear resumen de compilación
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    summary = f"""
    BUILD SUMMARY
    =============
    Timestamp: {timestamp}
    Environment: {env['PIOENV']}
    Board: {env['BOARD']}
    Platform: {env['PLATFORM']}
    """
    
    # Guardar resumen en archivo
    summary_file = "build_summary.txt"
    with open(summary_file, 'w') as f:
        f.write(summary)
    
    print(f"\nBuild summary saved to: {summary_file}")
    print("="*60 + "\n")
    print("✅ BUILD COMPLETED SUCCESSFULLY!")
    print("")

# Registrar la función
env.AddPostAction("buildprog", after_build)
