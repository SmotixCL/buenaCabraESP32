# 🐐 Collar BuenaCabra V3.0

## Sistema de Geofencing LoRaWAN para Ganado

### 📋 Descripción
Sistema de collar inteligente para monitoreo y control de ganado mediante GPS y LoRaWAN. Permite establecer geocercas virtuales y alertar cuando el animal sale del área designada.

### 🔧 Hardware
- **Placa**: Heltec WiFi LoRa 32 V3
- **MCU**: ESP32-S3FN8 @ 240MHz
- **Radio**: SX1262 LoRaWAN
- **GPS**: NEO-6M/NEO-8M
- **Display**: OLED 128x64
- **Buzzer**: Piezo activo
- **Batería**: LiPo 3.7V

### 📁 Estructura del Proyecto
```
Collar Buena Cabra/
├── src/
│   ├── main.cpp              # Archivo principal
│   ├── hardware/              # Drivers de hardware
│   │   ├── BuzzerManager.*    # Control del buzzer
│   │   ├── DisplayManager.*   # Control del display OLED
│   │   ├── GPSManager.*       # Gestión del GPS
│   │   ├── PowerManager.*     # Gestión de energía
│   │   └── RadioManager.*     # Comunicación LoRaWAN
│   ├── system/                # Lógica del sistema
│   │   ├── AlertManager.*     # Sistema de alertas
│   │   └── GeofenceManager.*  # Gestión de geocercas
│   ├── core/                  # Funciones core
│   │   ├── Logger.*           # Sistema de logging
│   │   └── Types.h            # Tipos de datos
│   └── utils/                 # Utilidades
├── include/
│   └── config.h               # Configuración global
├── lib/                       # Librerías locales
├── scripts/                   # Scripts de compilación
│   ├── pre_build.py          # Pre-compilación
│   └── post_build.py         # Post-compilación
├── platformio.ini             # Configuración PlatformIO
└── README.md                  # Este archivo
```

### 🚀 Instalación y Configuración

#### 1. Requisitos Previos
- [PlatformIO IDE](https://platformio.org/install/ide/vscode) instalado en VSCode
- Drivers USB-Serial para ESP32-S3 (CH340/CP2102)
- Python 3.x instalado

#### 2. Clonar el Repositorio
```bash
git clone https://github.com/tuusuario/collar-buenacabra.git
cd collar-buenacabra
```

#### 3. Configurar Credenciales LoRaWAN
Editar las credenciales en `src/main.cpp`:
```cpp
const uint8_t LORAWAN_DEV_EUI[8] = {0x01, 0x02, ...};
const uint8_t LORAWAN_APP_EUI[8] = {0x00, 0x00, ...};
const uint8_t LORAWAN_APP_KEY[16] = {0x2B, 0x7E, ...};
```

#### 4. Compilar y Cargar
```bash
# Compilar
pio run

# Cargar al dispositivo
pio run --target upload

# Monitor serial
pio device monitor
```

### 📡 Protocolo de Comunicación

#### Uplink (Collar → Servidor)
**Puerto 2 - Posición GPS** (12 bytes):
```
[0]     Tipo mensaje (0x01)
[1-4]   Latitud (float)
[5-8]   Longitud (float)
[9]     Batería (%)
[10]    Estado alerta (0-4)
[11]    Satélites GPS
```

#### Downlink (Servidor → Collar)
**Puerto 10 - Actualización Geocerca**:
```
[0]     Comando (0x01=actualizar)
[1-4]   Latitud centro (float)
[5-8]   Longitud centro (float)
[9-12]  Radio en metros (float)
[13-28] Nombre (string)
```

### 🔌 Conexiones de Hardware

| Componente | Pin ESP32-S3 | Descripción |
|------------|--------------|-------------|
| LoRa NSS   | GPIO 8       | Chip Select SPI |
| LoRa MOSI  | GPIO 10      | SPI MOSI |
| LoRa MISO  | GPIO 11      | SPI MISO |
| LoRa SCK   | GPIO 9       | SPI Clock |
| LoRa RST   | GPIO 12      | Reset |
| LoRa DIO1  | GPIO 14      | Interrupt |
| LoRa BUSY  | GPIO 13      | Busy signal |
| OLED SDA   | GPIO 17      | I2C Data |
| OLED SCL   | GPIO 18      | I2C Clock |
| OLED RST   | GPIO 21      | Reset |
| GPS RX     | GPIO 3       | Serial RX |
| GPS TX     | GPIO 4       | Serial TX |
| Buzzer     | GPIO 7       | PWM |
| LED        | GPIO 35      | Status LED |
| Button     | GPIO 0       | PRG Button |
| VBAT       | GPIO 1       | Battery ADC |
| VEXT       | GPIO 36      | Power control |

### 🎮 Uso del Sistema

#### Estados del LED:
- **Parpadeo rápido (5x)**: Sistema iniciando
- **Parpadeo lento (1x/10s)**: Sistema operacional
- **Parpadeo continuo**: Error del sistema
- **Encendido fijo**: Transmitiendo

#### Botón PRG:
- **Presión corta**: Cambiar pantalla
- **Presión larga (3s)**: Forzar transmisión

#### Pantallas del Display:
1. **Principal**: Estado general, GPS, batería
2. **GPS Detalle**: Coordenadas, satélites, precisión
3. **Geocerca**: Estado, distancia, alertas
4. **Estadísticas**: Paquetes, RSSI, SNR

### ⚠️ Niveles de Alerta

| Nivel | Distancia | Acción |
|-------|-----------|--------|
| SAFE | < 50m | Normal |
| CAUTION | 50-150m | Beep suave |
| WARNING | 150-300m | Beep frecuente |
| DANGER | 300-500m | Beep rápido |
| EMERGENCY | > 500m | Alarma continua |

### 🔍 Debugging

#### Monitor Serial
El dispositivo usa USB CDC para comunicación serial. Para ver los logs:
```bash
pio device monitor -b 115200
```

#### Comandos Serial:
- `status` - Muestra estado del sistema
- `gps` - Información GPS actual
- `fence` - Estado de geocerca
- `reset` - Reiniciar dispositivo

### 📊 Consumo de Energía

| Modo | Consumo | Duración (2000mAh) |
|------|---------|---------------------|
| Activo | ~150mA | 13 horas |
| GPS Fix | ~80mA | 25 horas |
| Sleep | ~20mA | 100 horas |
| Deep Sleep | ~5mA | 400 horas |

### 🐛 Solución de Problemas

#### No aparece el puerto serial:
1. Verificar drivers USB
2. Probar otro cable USB
3. Reiniciar con botón BOOT presionado

#### GPS no obtiene fix:
1. Verificar antena GPS
2. Salir al exterior
3. Esperar 3-5 minutos

#### LoRaWAN no conecta:
1. Verificar credenciales
2. Confirmar cobertura de gateway
3. Revisar configuración de región

### 📝 Licencia
MIT License - Ver archivo LICENSE

### 👥 Contribuidores
- Equipo BuenaCabra

### 📧 Contacto
Para soporte técnico: soporte@buenacabra.cl

---
**Versión**: 3.0.0  
**Fecha**: Enero 2025  
**Estado**: Prototipo Funcional
