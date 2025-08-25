# 📚 Guía de Desarrollo - Collar BuenaCabra V3.0

## 🏗️ Arquitectura del Sistema

### Capas del Software

```
┌─────────────────────────────────────┐
│         Aplicación Principal        │
│            (main.cpp)               │
├─────────────────────────────────────┤
│         Managers de Sistema         │
│   (AlertManager, GeofenceManager)   │
├─────────────────────────────────────┤
│        Managers de Hardware         │
│  (GPS, Radio, Display, Buzzer, Power)│
├─────────────────────────────────────┤
│            Core & Utils             │
│      (Logger, Types, Helpers)       │
├─────────────────────────────────────┤
│         HAL (Hardware Layer)        │
│      (Arduino, ESP32, RadioLib)     │
└─────────────────────────────────────┘
```

## 🔧 Configuración del Entorno

### 1. Instalar Herramientas

```bash
# Instalar PlatformIO CLI
pip install platformio

# Verificar instalación
pio --version

# Actualizar plataformas y librerías
pio update
pio lib update
```

### 2. Configurar Git

```bash
# Configurar usuario
git config --global user.name "Tu Nombre"
git config --global user.email "tu@email.com"

# Clonar repositorio
git clone https://github.com/tuusuario/collar-buenacabra.git
cd collar-buenacabra

# Crear rama de desarrollo
git checkout -b feature/mi-caracteristica
```

## 📝 Estándares de Código

### Nomenclatura

- **Clases**: PascalCase → `DisplayManager`
- **Funciones**: camelCase → `updateDisplay()`
- **Variables**: camelCase → `batteryLevel`
- **Constantes**: UPPER_SNAKE_CASE → `MAX_RETRY_COUNT`
- **Archivos**: PascalCase → `RadioManager.cpp`

### Comentarios

```cpp
/**
 * @brief Descripción breve de la función
 * @param param1 Descripción del parámetro
 * @return Descripción del retorno
 */
int miFuncion(int param1) {
    // Comentario de línea para lógica compleja
    return param1 * 2;
}
```

### Estructura de Archivos

#### Header (.h)
```cpp
#pragma once
#include <Arduino.h>

class MiClase {
public:
    MiClase();
    void metodoPublico();
    
private:
    int variablePrivada;
    void metodoPrivado();
};
```

#### Implementation (.cpp)
```cpp
#include "MiClase.h"

MiClase::MiClase() : variablePrivada(0) {
    // Constructor
}

void MiClase::metodoPublico() {
    // Implementación
}
```

## 🔄 Flujo de Trabajo Git

### Ramas

- `main` - Versión estable de producción
- `develop` - Desarrollo activo
- `feature/*` - Nuevas características
- `bugfix/*` - Corrección de bugs
- `hotfix/*` - Correcciones urgentes

### Commits

```bash
# Formato de mensaje
git commit -m "tipo: descripción breve

Descripción detallada opcional"

# Tipos:
# feat: Nueva característica
# fix: Corrección de bug
# docs: Cambios en documentación
# style: Formato de código
# refactor: Refactorización
# test: Añadir tests
# chore: Tareas de mantenimiento
```

## 🧪 Testing

### Ejecutar Tests

```bash
# Todos los tests
pio test

# Test específico
pio test -f test_hardware

# Con verbose
pio test -v
```

### Escribir Tests

```cpp
#include <unity.h>

void test_mi_funcion() {
    // Arrange
    int input = 5;
    
    // Act
    int result = miFuncion(input);
    
    // Assert
    TEST_ASSERT_EQUAL(10, result);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_mi_funcion);
    UNITY_END();
}
```

## 🐛 Debugging

### Monitor Serial

```bash
# Monitor básico
pio device monitor

# Con filtros
pio device monitor -f colorize -f time

# Con baudrate específico
pio device monitor -b 115200
```

### Debug con GDB

```bash
# Compilar con símbolos de debug
pio run -e debug

# Iniciar debug
pio debug

# Comandos GDB útiles:
# break main.cpp:100  - Breakpoint en línea 100
# continue            - Continuar ejecución
# step                - Siguiente línea
# print variable      - Imprimir valor
# backtrace           - Ver stack trace
```

### Logs de Debug

```cpp
// Niveles de log
LOG_E("Error: %s", errorMsg);      // Error
LOG_W("Warning: %d", value);       // Warning
LOG_I("Info: %s", info);           // Info
LOG_D("Debug: %f", debugValue);    // Debug

// Log condicional
#ifdef DEBUG_MODE
    Serial.printf("Debug: Estado = %d\n", estado);
#endif
```

## 📦 Gestión de Librerías

### Añadir Librería

```bash
# Buscar librería
pio lib search radiolib

# Instalar librería
pio lib install "jgromes/RadioLib@^6.6.0"

# O añadir en platformio.ini:
lib_deps = 
    jgromes/RadioLib@^6.6.0
```

### Actualizar Librerías

```bash
# Ver librerías instaladas
pio lib list

# Actualizar todas
pio lib update

# Actualizar específica
pio lib update RadioLib
```

## 🚀 Compilación y Deploy

### Compilación

```bash
# Compilar proyecto
pio run

# Compilar para entorno específico
pio run -e heltec_wifi_lora_32_v3

# Limpiar y compilar
pio run -t clean
pio run
```

### Upload

```bash
# Cargar firmware
pio run -t upload

# Especificar puerto
pio run -t upload --upload-port COM5

# Upload y monitor
pio run -t upload && pio device monitor
```

### OTA (Over The Air)

```cpp
// Configurar en código
#include <ArduinoOTA.h>

void setupOTA() {
    ArduinoOTA.setHostname("collar-buenacabra");
    ArduinoOTA.setPassword("admin");
    
    ArduinoOTA.onStart([]() {
        Serial.println("OTA Update Starting...");
    });
    
    ArduinoOTA.begin();
}

// En loop()
ArduinoOTA.handle();
```

## 📊 Optimización

### Memoria

```cpp
// Usar PROGMEM para constantes
const char message[] PROGMEM = "Mensaje en flash";

// Usar F() macro para strings
Serial.println(F("String en flash"));

// Verificar memoria disponible
Serial.printf("Heap libre: %d bytes\n", ESP.getFreeHeap());
```

### Energía

```cpp
// Deep sleep
esp_sleep_enable_timer_wakeup(60 * 1000000); // 60 segundos
esp_deep_sleep_start();

// Light sleep
esp_sleep_enable_timer_wakeup(10 * 1000000);
esp_light_sleep_start();

// Reducir frecuencia CPU
setCpuFrequencyMhz(80); // 80MHz en lugar de 240MHz
```

## 🔍 Troubleshooting Común

### Problema: No se detecta el puerto serial

```bash
# Linux/Mac: Dar permisos
sudo chmod 666 /dev/ttyUSB0

# Windows: Instalar drivers
# Descargar de: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
```

### Problema: Error de compilación

```bash
# Limpiar cache
pio run -t clean
rm -rf .pio

# Reinstalar plataforma
pio platform uninstall espressif32
pio platform install espressif32
```

### Problema: Upload falla

```bash
# Reducir velocidad de upload
# En platformio.ini:
upload_speed = 115200

# Modo bootloader manual:
# 1. Mantener BOOT presionado
# 2. Presionar y soltar RST
# 3. Soltar BOOT
# 4. Ejecutar upload
```

## 📚 Recursos Adicionales

### Documentación

- [PlatformIO Docs](https://docs.platformio.org/)
- [ESP32 Reference](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [RadioLib Wiki](https://github.com/jgromes/RadioLib/wiki)
- [LoRaWAN Spec](https://lora-alliance.org/resource_hub/lorawan-specification-v1-0-3/)

### Herramientas Útiles

- [Serial Plot](https://github.com/CieNTi/serial_plot) - Visualizar datos serial
- [ChirpStack](https://www.chirpstack.io/) - Servidor LoRaWAN
- [MQTT Explorer](http://mqtt-explorer.com/) - Cliente MQTT
- [Wireshark](https://www.wireshark.org/) - Análisis de red

### Comunidad

- [PlatformIO Community](https://community.platformio.org/)
- [ESP32 Forum](https://www.esp32.com/)
- [LoRaWAN Forum](https://www.thethingsnetwork.org/forum/)

---

**Última actualización**: Enero 2025  
**Versión del documento**: 1.0.0
