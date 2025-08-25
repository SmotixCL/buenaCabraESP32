# 📋 Changelog - Collar BuenaCabra

Todos los cambios notables en este proyecto serán documentados en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.0.0] - 2025-01-XX

### 🎉 Nueva Versión Mayor - Reestructuración Completa

### ✨ Agregado
- Nueva arquitectura modular con managers separados
- Sistema de alertas multinivel (SAFE, CAUTION, WARNING, DANGER, EMERGENCY)
- Soporte para geocercas poligonales (además de circulares)
- Monitor serial mejorado con USB CDC para ESP32-S3
- Scripts de pre y post compilación
- Tests unitarios básicos
- Documentación completa de desarrollo
- Ejemplo de archivo de credenciales
- Sistema de logging mejorado
- Callback para actualizaciones de geocerca vía downlink
- Múltiples pantallas en display OLED (navegación con botón)

### 🔄 Cambiado
- Migración completa a ESP32-S3 (Heltec V3)
- Actualización de RadioLib a v6.6.0
- Refactorización completa del código principal
- Mejora en la gestión de energía
- Optimización del uso de memoria
- Estructura de directorios reorganizada
- Configuración platform.ini optimizada para USB CDC

### 🐛 Corregido
- Monitor serial que no funcionaba con ESP32-S3
- Problemas de inicialización de hardware
- Memory leaks en gestión de strings
- Timing issues en comunicación LoRaWAN
- Configuración incorrecta de pines para Heltec V3

### 🗑️ Eliminado
- Archivos duplicados y no utilizados
- Código legacy de versiones anteriores
- Dependencias obsoletas

## [2.0.0] - 2024-XX-XX

### ✨ Agregado
- Soporte inicial para LoRaWAN
- Sistema básico de geocercas
- Display OLED

### 🔄 Cambiado
- Migración de LoRa P2P a LoRaWAN
- Actualización de librerías

## [1.0.0] - 2024-XX-XX

### ✨ Agregado
- Versión inicial del proyecto
- Comunicación LoRa básica
- Lectura de GPS
- Control de buzzer

---

## Leyenda de Emojis

- ✨ `Agregado` - Nueva funcionalidad
- 🔄 `Cambiado` - Cambios en funcionalidad existente
- 🗑️ `Eliminado` - Funcionalidad removida
- 🐛 `Corregido` - Corrección de bugs
- 🔒 `Seguridad` - Mejoras de seguridad
- ⚡ `Rendimiento` - Mejoras de rendimiento
- 📝 `Documentación` - Cambios en documentación
- 🎨 `Estilo` - Cambios de formato/estructura
- ♻️ `Refactor` - Refactorización de código
- 🧪 `Tests` - Adición o modificación de tests
- 🔧 `Configuración` - Cambios en configuración
- 🚀 `Deploy` - Cambios relacionados con deployment
