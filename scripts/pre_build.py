#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Pre-build script for Collar BuenaCabra
Ejecuta antes de compilar el proyecto
"""

Import("env")
import os
from datetime import datetime

def before_build(source, target, env):
    """Función ejecutada antes de la compilación"""
    
    print("\n" + "="*60)
    print("🐐 COLLAR BUENACABRA - PRE-BUILD SCRIPT")
    print("="*60)
    
    # Información del entorno
    print(f"Build Environment: {env.get('PIOENV', 'unknown')}")
    print(f"Platform: {env.get('PLATFORM', 'unknown')}")
    print(f"Board: {env.get('BOARD', 'unknown')}")
    # Framework puede no estar disponible en este contexto
    if 'FRAMEWORK' in env:
        print(f"Framework: {env['FRAMEWORK']}")
    else:
        print(f"Framework: arduino")  # Valor por defecto
    
    # Timestamp de compilación
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"Build Timestamp: {timestamp}")
    
    # Verificar estructura de directorios
    directories = ['src', 'include', 'lib']
    for dir in directories:
        if os.path.exists(dir):
            print(f"✓ Directory '{dir}' exists")
        else:
            print(f"✗ Directory '{dir}' missing!")
    
    # Definir macros de compilación adicionales
    env.Append(CPPDEFINES=[
        ("BUILD_TIMESTAMP", f'\\"{timestamp}\\"'),
        ("BUILD_ENV", f'\\"{env["PIOENV"]}\\"')
    ])
    
    print("="*60 + "\n")

# Registrar la función
env.AddPreAction("buildprog", before_build)
