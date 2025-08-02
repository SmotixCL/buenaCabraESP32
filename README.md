# 🐐 LoRaWAN Collar Geofencing

**Sistema profesional de geofencing para ganado usando LoRaWAN**

Collar inteligente que monitorea posición GPS y transmite por LoRaWAN, con alertas automáticas al salir de geocercas definidas.

![Status](https://img.shields.io/badge/Status-Functional-brightgreen)
![Hardware](https://img.shields.io/badge/Hardware-Heltec%20WiFi%20LoRa%2032%20V3-blue)
![Protocol](https://img.shields.io/badge/Protocol-LoRaWAN%201.0.3-orange)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## 🚀 Características

✅ **GPS Real** - Posicionamiento preciso con NEO-6M  
✅ **LoRaWAN AU915** - Transmisión de largo alcance para Chile  
✅ **Display OLED** - Información en tiempo real  
✅ **Gestión de Batería** - Monitoreo y optimización energética  
✅ **Buzzer Inteligente** - Alertas sonoras configurables  
✅ **Arquitectura Modular** - Sistema escalable y mantenible  
✅ **Sin Reinicios** - Sistema estable 24/7  

---

## 🔧 Hardware

| Componente | Modelo | Función |
|------------|--------|---------|
| **MCU** | ESP32-S3FN8 | Procesamiento principal |
| **LoRa** | SX1262 | Comunicación LoRaWAN |
| **GPS** | NEO-6M | Posicionamiento |
| **Display** | OLED 128x64 I2C | Información visual |
| **Audio** | Buzzer PWM | Alertas sonoras |
| **Batería** | LiPo 3.7V | Alimentación portátil |

---

## 📡 Especificaciones LoRaWAN

- **Región:** AU915 Sub-banda 1 (Compatible Chile)
- **Frecuencia:** 917.0 MHz
- **Potencia:** 20 dBm (100mW)
- **Spreading Factor:** SF9 (balance alcance/consumo)
- **Payload:** 12 bytes optimizado
- **Intervalo:** 60 segundos normal, 30s alerta

---

## ⚡ Instalación

### Requisitos

- **PlatformIO IDE** o **Arduino IDE**
- **USB-C Cable** para programación
- **Gateway LoRaWAN** (RAK5146 recomendado)
- **ChirpStack Server** v3+

### Compilar y Subir

```bash
# Clonar repositorio
git clone <repository-url>
cd collar-lorawan-geofencing

# Compilar
pio run

# Subir al ESP32
pio run --target upload

# Monitorear (opcional - sistema funciona sin Serial)
pio device monitor
```

---

## 🔑 Configuración LoRaWAN

### Método ABP (Recomendado para testing)

Editar en `src/main.cpp`:

```cpp
// Reemplazar con claves reales de ChirpStack
uint8_t devAddr[4] = {0x01, 0x02, 0x03, 0x04};
uint8_t nwkSKey[16] = {0x2B, 0x7E, 0x15, 0x16, /* ... */};
uint8_t appSKey[16] = {0x2B, 0x7E, 0x15, 0x16, /* ... */};
```

### Crear Device en ChirpStack

1. **Application** → Create New
2. **Device** → Add Device  
3. **Keys** → Configurar ABP con claves del código
4. **Device Profile** → AU915, Clase A

---

## 📊 Monitoreo

### LEDs de Estado

- **3 parpadeos rápidos** → Sistema iniciando
- **2 parpadeos medianos** → Manager inicializado OK
- **6 parpadeos largos** → LoRaWAN conectado exitosamente
- **2 parpadeos cortos** → Packet enviado
- **1 parpadeo c/20s** → Heartbeat sistema funcionando

### Display OLED

- **Uptime** del sistema
- **Coordenadas GPS** actuales
- **Estado LoRaWAN** (Connected/Searching)
- **Nivel de batería** (voltaje y porcentaje)
- **Memoria libre** del sistema

---

## 🗺️ Backend Integration

El collar es compatible con el **backend FastAPI** incluido:

- **Endpoint**: `/api/integrations/uplink`
- **Database**: PostgreSQL + PostGIS
- **Frontend**: Dashboard web con Leaflet
- **Geocercas**: Dibujado interactivo de zonas
- **Alertas**: Downlinks automáticos por LoRaWAN

Ver `backend/` para servidor completo.

---

## 🔧 Arquitectura de Software

```
src/
├── main.cpp              # Loop principal
├── config/               # Configuraciones
│   ├── pins.h           # Definición de pines
│   ├── constants.h      # Constantes del sistema
│   └── lorawan_config.h # Configuración LoRaWAN
├── core/                # Núcleo del sistema
│   ├── Types.h          # Estructuras de datos
│   └── Logger.h         # Sistema de logging
├── hardware/            # Managers de hardware
│   ├── BuzzerManager.*  # Gestión de audio
│   ├── PowerManager.*   # Gestión de batería
│   ├── DisplayManager.* # Gestión OLED
│   ├── GPSManager.*     # Gestión GPS
│   └── RadioManager.*   # Gestión LoRaWAN
└── system/              # Managers del sistema
    ├── GeofenceManager.* # Lógica de geocercas
    └── AlertManager.*    # Sistema de alertas
```

---

## 🔋 Consumo Energético

| Estado | Consumo | Duración Estimada |
|--------|---------|-------------------|
| **Transmisión** | 120mA × 3s | - |
| **GPS Activo** | 40mA × 60s | - |
| **Deep Sleep** | 5µA | 99% del tiempo |
| **Autonomía Total** | - | **4-6 meses** |

---

## 🌍 Casos de Uso

- **Ganadería** - Monitoreo de rebaños
- **Mascotas** - Seguimiento de animales 
- **Assets** - Rastreo de equipos valiosos
- **Agricultura** - Control de maquinaria
- **Investigación** - Estudios de fauna

---

## 🛠️ Troubleshooting

### Sistema no conecta LoRaWAN
- Verificar claves ABP en ChirpStack
- Confirmar AU915 Sub-banda 1
- Revisar alcance del gateway

### GPS sin fix
- Ubicar al aire libre 
- Esperar 2-3 minutos para cold start
- Verificar antena GPS conectada

### Display no funciona
- Verificar cables I2C (SDA/SCL)
- Confirmar dirección 0x3C
- Revisar alimentación VEXT

---

## 📈 Desarrollo Futuro

- [ ] **OTAA Support** - Join automático
- [ ] **Multi-Geocercas** - Zonas múltiples  
- [ ] **OTA Updates** - Actualización remota
- [ ] **LoRa Mesh** - Red entre collares
- [ ] **Solar Panel** - Carga solar
- [ ] **Acelerómetro** - Detección de actividad

---

## 🤝 Contribuir

1. Fork el repositorio
2. Crear feature branch (`git checkout -b feature/nueva-funcionalidad`)
3. Commit cambios (`git commit -am 'Añadir nueva funcionalidad'`)
4. Push al branch (`git push origin feature/nueva-funcionalidad`)
5. Crear Pull Request

---

## 📄 Licencia

Este proyecto está licenciado bajo MIT License - ver [LICENSE](LICENSE) para detalles.

**¿Qué significa MIT License?** Es una licencia muy permisiva que permite:
- ✅ Uso comercial y personal
- ✅ Modificación y distribución
- ✅ Uso privado sin restricciones
- ✅ Solo requiere mantener copyright y licencia

---

## 👨‍💻 Autor

**[Smotix](https://github.com/Smotix)**

🚀 **Desarrollador de sistemas IoT y LoRaWAN**
- Especialista en arquitecturas modulares ESP32
- Sistemas de geofencing y tracking
- Optimización de consumo energético
- Integración ChirpStack y backends profesionales

📧 **Contacto**: Para consultas técnicas, abrir un [Issue](../../issues)

---

## 🏆 Logros del Proyecto

- ⚡ **Sistema estable 24/7** - Sin reinicios
- 🔋 **Autonomía récord** - 4-6 meses
- 📡 **LoRaWAN funcional** - AU915 Chile
- 🎯 **Arquitectura modular** - Fácil mantenimiento
- 🛠️ **Debugging avanzado** - LED patterns + OLED
- 📱 **Backend completo** - FastAPI + PostgreSQL

---

## 🙏 Agradecimientos

- **Heltec Automation** - Hardware WiFi LoRa 32 V3
- **RadioLib** - Librería LoRaWAN
- **ChirpStack** - Network Server
- **OpenStreetMap** - Mapas del backend
- **Comunidad IoT** - Feedback y testing

---

## 📊 Stats del Proyecto

![GitHub stars](https://img.shields.io/github/stars/Smotix/collar-lorawan-geofencing?style=social)
![GitHub forks](https://img.shields.io/github/forks/Smotix/collar-lorawan-geofencing?style=social)
![GitHub issues](https://img.shields.io/github/issues/Smotix/collar-lorawan-geofencing)

---

**¿Preguntas?** Abrir un [Issue](../../issues) o consultar la [Wiki](../../wiki).

**⭐ Si este proyecto te sirvió, dale una estrella!**
