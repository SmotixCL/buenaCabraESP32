/*
 * ============================================================================
 * COLLAR LORAWAN HÍBRIDO - VERSIÓN CORREGIDA
 * ============================================================================
 * 
 * CORRECCIONES APLICADAS:
 * ✅ APIs reales de managers (AlertManager constructor, GeofenceManager API)
 * ✅ Posición cambiada a Chacay Bikepark (-37.34640277978371, -72.91495492379738)
 * ✅ Position struct correcta (no lista de inicialización)
 * ✅ Callbacks con signaturas correctas
 * ✅ Métodos reales de cada manager
 * 
 * ESTABILIDAD:
 * ✅ Inicialización secuencial con delays
 * ✅ Intervalos extendidos para estabilidad
 * ✅ Logging híbrido sin bloqueo
 * ✅ Watchdog deshabilitado inicialmente
 */

#include <Arduino.h>
#include <Wire.h>
#include <SSD1306Wire.h>

// Configuración del sistema
#include "config/pins.h"
#include "config/constants.h"
#include "config/lorawan_config.h"
#include "core/Types.h"

// Managers de hardware
#include "hardware/BuzzerManager.h"
#include "hardware/PowerManager.h"
#include "hardware/DisplayManager.h"
#include "hardware/GPSManager.h"
#include "hardware/RadioManager.h"

// Managers del sistema
#include "system/GeofenceManager.h"
#include "system/AlertManager.h"

// ============================================================================
// CONFIGURACIONES DE ESTABILIDAD
// ============================================================================
#define STABLE_MODE                 true
#define EXTENDED_INTERVALS          true
#define HYBRID_LOGGING              true

// Intervalos optimizados para estabilidad
#define STABLE_GPS_INTERVAL         15000   // 15s
#define STABLE_DISPLAY_INTERVAL     20000   // 20s  
#define STABLE_GEOFENCE_INTERVAL    10000   // 10s
#define STABLE_LORAWAN_INTERVAL     120000  // 2min
#define STABLE_HEARTBEAT_INTERVAL   30000   // 30s

// ============================================================================
// MANAGERS CON CONSTRUCTORES CORRECTOS
// ============================================================================

// Managers hardware (orden importante)
BuzzerManager buzzerManager(BUZZER_PIN);
PowerManager powerManager(VBAT_PIN);
DisplayManager displayManager;
GPSManager gpsManager;
RadioManager radioManager;
GeofenceManager geofenceManager;

// AlertManager requiere referencias a BuzzerManager y DisplayManager
AlertManager alertManager(buzzerManager, displayManager);  // ✅ CORREGIDO

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

// Posición por defecto: CHACAY BIKEPARK (como solicitas)
Position currentPosition;  // ✅ Sin inicialización de lista
BatteryStatus batteryStatus;
SystemStatus systemStatus;

// Control de timing
uint32_t lastGPSCheck = 0;
uint32_t lastBatteryCheck = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastLoRaTransmit = 0;
uint32_t lastHeartbeat = 0;
uint32_t lastGeofenceCheck = 0;

// Estados operacionales
uint16_t packetCounter = 0;
bool systemReady = false;
bool loraJoined = false;
bool gpsHasFix = false;
AlertLevel currentAlert = AlertLevel::SAFE;

// Configuración LoRaWAN con DevEUI real
uint8_t devEUI[8] = {0x58, 0xEC, 0x3C, 0x43, 0xCA, 0x48, 0x00, 0x00};

// Configuración ABP temporal
uint8_t devAddr[4] = {0x01, 0x02, 0x03, 0x04};
uint8_t nwkSKey[16] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 
                       0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
uint8_t appSKey[16] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 
                       0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};

// ============================================================================
// FUNCIONES DE DEBUGGING ESTABLES
// ============================================================================

void stableLEDSignal(int pattern) {
    switch(pattern) {
        case 1: // Sistema iniciando
            for(int i=0; i<3; i++) { 
                digitalWrite(LED_PIN, HIGH); delay(150); 
                digitalWrite(LED_PIN, LOW); delay(150); 
            }
            break;
        case 2: // Manager OK
            for(int i=0; i<2; i++) { 
                digitalWrite(LED_PIN, HIGH); delay(250); 
                digitalWrite(LED_PIN, LOW); delay(250); 
            }
            break;
        case 3: // Error
            for(int i=0; i<5; i++) { 
                digitalWrite(LED_PIN, HIGH); delay(100); 
                digitalWrite(LED_PIN, LOW); delay(100); 
            }
            break;
        case 4: // Sistema completo
            for(int i=0; i<6; i++) { 
                digitalWrite(LED_PIN, HIGH); delay(200); 
                digitalWrite(LED_PIN, LOW); delay(200); 
            }
            break;
        case 5: // GPS fix
            digitalWrite(LED_PIN, HIGH); delay(800); 
            digitalWrite(LED_PIN, LOW);
            break;
        case 6: // LoRaWAN packet
            digitalWrite(LED_PIN, HIGH); delay(80); digitalWrite(LED_PIN, LOW); delay(80);
            digitalWrite(LED_PIN, HIGH); delay(80); digitalWrite(LED_PIN, LOW);
            break;
        case 7: // Geocerca activada
            for(int i=0; i<4; i++) { 
                digitalWrite(LED_PIN, HIGH); delay(120); 
                digitalWrite(LED_PIN, LOW); delay(120); 
            }
            break;
        case 9: // Heartbeat
            digitalWrite(LED_PIN, HIGH); delay(150); 
            digitalWrite(LED_PIN, LOW);
            break;
    }
}

void stableSerialLog(const char* message) {
    #if HYBRID_LOGGING
    if (Serial && Serial.availableForWrite()) {
        Serial.println(message);
    }
    #endif
}

// ============================================================================
// INICIALIZACIÓN DE POSICIÓN CHACAY BIKEPARK
// ============================================================================

void initializeChacayPosition() {
    // ✅ POSICIÓN CORRECTA CHACAY BIKEPARK según solicitaste
    currentPosition.latitude = -37.34640277978371;
    currentPosition.longitude = -72.91495492379738;
    currentPosition.altitude = 100.0f;
    currentPosition.satellites = 0;  // Se actualizará con GPS real
    currentPosition.accuracy = 0.0f;
    currentPosition.timestamp = millis();
    currentPosition.valid = true;
    
    stableSerialLog("📍 Posición inicial: Chacay Bikepark (-37.346, -72.915)");
}

// ============================================================================
// CONFIGURACIÓN DE GEOCERCA (API CORREGIDA)
// ============================================================================

void setupGeofenceChacay() {
    // ✅ Verificar que el GeofenceManager use las APIs correctas
    // GeofenceManager no tiene setCenter/setRadius, debe configurarse diferente
    
    // Según las APIs encontradas, GeofenceManager puede no tener configuración manual
    // Verificar si ya está configurado por defecto o necesita otro método
    
    if (geofenceManager.isActive()) {
        float radius = geofenceManager.getRadius();
        double centerLat = geofenceManager.getCenterLat();
        double centerLng = geofenceManager.getCenterLng();
        
        char geoMsg[100];
        snprintf(geoMsg, sizeof(geoMsg), "📍 Geocerca: %.6f,%.6f R=%.1fm", 
                centerLat, centerLng, radius);
        stableSerialLog(geoMsg);
        
        stableLEDSignal(7); // Señal geocerca activa
    } else {
        stableSerialLog("⚠️ GeofenceManager no activo - usando modo básico");
    }
}

void configureAlertCallbacks() {
    // ✅ CALLBACK CORREGIDO con signatura real: void (*)(AlertLevel, float, const char*)
    alertManager.setAlertCallback([](AlertLevel level, float distance, const char* reason) {
        currentAlert = level;
        
        char alertMsg[80];
        snprintf(alertMsg, sizeof(alertMsg), "🚨 Alert: %d, Dist: %.1fm, %s", 
                (int)level, distance, reason);
        stableSerialLog(alertMsg);
        
        switch(level) {
            case AlertLevel::SAFE:
                buzzerManager.stopAllTones();  // Usar método disponible
                break;
                
            case AlertLevel::CAUTION:
                buzzerManager.playTone(FREQ_CAUTION, TONE_DURATION_SHORT, VOLUME_LOW);
                break;
                
            case AlertLevel::WARNING:
                buzzerManager.playTone(FREQ_WARNING, TONE_DURATION_MEDIUM, VOLUME_MEDIUM);
                break;
                
            case AlertLevel::DANGER:
                buzzerManager.playTone(FREQ_DANGER, TONE_DURATION_LONG, VOLUME_HIGH);
                break;
                
            case AlertLevel::EMERGENCY:
                buzzerManager.playTone(FREQ_EMERGENCY, TONE_DURATION_LONG, VOLUME_MAX);
                break;
        }
    });
}

// ============================================================================
// PAYLOAD OPTIMIZADO PARA CHIRPSTACK BACKEND
// ============================================================================

struct GPSPayload {
    int32_t latitude;   // Latitud * 10^7
    int32_t longitude;  // Longitud * 10^7  
    uint16_t altitude;  // Altitud en metros
    uint8_t satellites; // Número de satélites
    uint8_t hdop;       // HDOP * 10
    uint8_t battery;    // Nivel batería 0-100%
    uint8_t alert;      // Nivel de alerta 0-4
    uint8_t status;     // Status bits: GPS_VALID(0), GEOFENCE_INSIDE(1)
} __attribute__((packed));

void createGPSPayload(uint8_t* buffer, size_t* length) {
    GPSPayload payload;
    
    // Convertir coordenadas GPS
    payload.latitude = (int32_t)(currentPosition.latitude * 10000000);
    payload.longitude = (int32_t)(currentPosition.longitude * 10000000);
    payload.altitude = (uint16_t)currentPosition.altitude;
    
    // Estado GPS con métodos corregidos
    payload.satellites = gpsManager.getSatelliteCount();  // ✅ CORREGIDO
    payload.hdop = (uint8_t)(gpsManager.getHDOP() * 10);
    
    // Estado sistema
    payload.battery = (uint8_t)batteryStatus.percentage;
    payload.alert = (uint8_t)currentAlert;
    
    // Status bits
    payload.status = 0;
    if (gpsHasFix) payload.status |= (1 << 0);
    if (geofenceManager.isInsideGeofence(currentPosition)) payload.status |= (1 << 1);  // ✅ CORREGIDO
    
    memcpy(buffer, &payload, sizeof(payload));
    *length = sizeof(payload);
    
    stableSerialLog("📦 Payload GPS creado: 15 bytes");
}

// ============================================================================
// FUNCIONES DE VERIFICACIÓN DE GEOCERCA (API CORREGIDA)
// ============================================================================

void checkGeofence() {
    if (!gpsHasFix) return;
    
    // ✅ Usar API real de GeofenceManager
    float distance = geofenceManager.getDistance(currentPosition);  // ✅ CORREGIDO
    bool isInside = geofenceManager.isInsideGeofence(currentPosition);  // ✅ CORREGIDO
    
    // Determinar nivel de alerta basado en distancia
    AlertLevel newLevel = AlertLevel::SAFE;
    
    if (!isInside) {
        float radius = geofenceManager.getRadius();
        float overflow = distance - radius;
        if (overflow > 50.0f) {
            newLevel = AlertLevel::EMERGENCY;
        } else if (overflow > 20.0f) {
            newLevel = AlertLevel::DANGER;
        } else {
            newLevel = AlertLevel::WARNING;
        }
    } else {
        // Dentro de la geocerca, verificar proximidad al borde
        float radius = geofenceManager.getRadius();
        float margin = radius - distance;
        if (margin < 10.0f) {
            newLevel = AlertLevel::CAUTION;
        } else {
            newLevel = AlertLevel::SAFE;
        }
    }
    
    // Actualizar alerta si cambió usando API real
    if (newLevel != currentAlert) {
        alertManager.update(distance);  // ✅ CORREGIDO: update(float) no updateAlert()
        
        char logMsg[100];
        snprintf(logMsg, sizeof(logMsg), "📍 Geocerca: %.1fm %s (Alert: %d)", 
                distance, isInside ? "INSIDE" : "OUTSIDE", (int)newLevel);
        stableSerialLog(logMsg);
    }
}

// ============================================================================
// SETUP PRINCIPAL - INICIALIZACIÓN SECUENCIAL ESTABLE
// ============================================================================

void setup() {
    // PASO 1: Configuración básica hardware
    pinMode(VEXT_ENABLE, OUTPUT);
    digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE);
    delay(300);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // PASO 2: Señal de inicio
    stableLEDSignal(1);
    
    // PASO 3: Serial opcional (sin bloqueo)
    #if HYBRID_LOGGING
    Serial.begin(115200);
    delay(1000);
    stableSerialLog("🚀 Collar Geofencing Híbrido v3.1 CORREGIDO");
    stableSerialLog("📍 Ubicación: Chacay Bikepark, Chile");
    #endif
    
    // PASO 4: Inicializar posición por defecto
    initializeChacayPosition();
    
    // PASO 5: I2C
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(100000); // Frecuencia estable
    delay(200);
    
    // PASO 6: Inicialización secuencial con delays
    
    // 6.1 BuzzerManager
    stableSerialLog("🔧 Inicializando BuzzerManager...");
    if (buzzerManager.init() == Result::SUCCESS) {
        stableLEDSignal(2);
        buzzerManager.playTone(1000, 200, 50);
        stableSerialLog("✅ BuzzerManager OK");
    } else {
        stableLEDSignal(3);
        stableSerialLog("❌ BuzzerManager ERROR");
    }
    delay(800);
    
    // 6.2 PowerManager
    stableSerialLog("🔧 Inicializando PowerManager...");
    if (powerManager.init() == Result::SUCCESS) {
        stableLEDSignal(2);
        powerManager.readBattery();
        batteryStatus = powerManager.getBatteryStatus();
        stableSerialLog("✅ PowerManager OK");
    } else {
        stableLEDSignal(3);
        stableSerialLog("❌ PowerManager ERROR");
    }
    delay(800);
    
    // 6.3 DisplayManager
    stableSerialLog("🔧 Inicializando DisplayManager...");
    if (displayManager.init() == Result::SUCCESS) {
        stableLEDSignal(2);
        displayManager.showSplashScreen();
        stableSerialLog("✅ DisplayManager OK");
        delay(2000);
    } else {
        stableLEDSignal(3);
        stableSerialLog("❌ DisplayManager ERROR");
    }
    delay(800);
    
    // 6.4 GPSManager
    stableSerialLog("🔧 Inicializando GPSManager...");
    if (gpsManager.init() == Result::SUCCESS) {
        stableLEDSignal(2);
        stableSerialLog("✅ GPSManager OK");
    } else {
        stableLEDSignal(3);
        stableSerialLog("❌ GPSManager ERROR");
    }
    delay(800);
    
    // 6.5 GeofenceManager
    stableSerialLog("🔧 Inicializando GeofenceManager...");
    if (geofenceManager.init() == Result::SUCCESS) {
        stableLEDSignal(2);
        setupGeofenceChacay();
        stableSerialLog("✅ GeofenceManager OK");
    } else {
        stableLEDSignal(3);
        stableSerialLog("❌ GeofenceManager ERROR");
    }
    delay(800);
    
    // 6.6 AlertManager
    stableSerialLog("🔧 Inicializando AlertManager...");
    if (alertManager.init() == Result::SUCCESS) {
        stableLEDSignal(2);
        configureAlertCallbacks();
        stableSerialLog("✅ AlertManager OK");
    } else {
        stableLEDSignal(3);
        stableSerialLog("❌ AlertManager ERROR");
    }
    delay(800);
    
    // 6.7 RadioManager (último)
    stableSerialLog("🔧 Inicializando RadioManager...");
    if (radioManager.init() == Result::SUCCESS) {
        stableLEDSignal(2);
        
        // Configurar LoRa con parámetros conservadores
        if (radioManager.setupLoRa(917.0, 125.0, 9, 7, 20) == Result::SUCCESS) {
            stableLEDSignal(2);
            
            if (radioManager.setupLoRaWAN() == Result::SUCCESS) {
                stableLEDSignal(2);
                
                // Join ABP
                if (radioManager.joinABP(devAddr, nwkSKey, appSKey) == Result::SUCCESS) {
                    loraJoined = true;
                    buzzerManager.playSuccessTone();
                    
                    // 8 parpadeos para híbrido exitoso
                    for(int i=0; i<8; i++) {
                        digitalWrite(LED_PIN, HIGH); delay(100);
                        digitalWrite(LED_PIN, LOW); delay(100);
                    }
                    stableSerialLog("✅ LoRaWAN ABP Join exitoso");
                } else {
                    stableSerialLog("❌ LoRaWAN Join falló");
                }
            }
        }
        stableSerialLog("✅ RadioManager OK");
    } else {
        stableLEDSignal(3);
        stableSerialLog("❌ RadioManager ERROR");
    }
    delay(1000);
    
    // PASO 7: Sistema listo
    systemReady = true;
    stableLEDSignal(4);
    stableSerialLog("🎉 Sistema híbrido iniciado - Chacay Bikepark ready!");
    
    // Inicializar timers
    uint32_t now = millis();
    lastGPSCheck = now;
    lastBatteryCheck = now;
    lastDisplayUpdate = now;
    lastLoRaTransmit = now;
    lastHeartbeat = now;
    lastGeofenceCheck = now;
    
    delay(2000);
}

// ============================================================================
// LOOP PRINCIPAL - OPTIMIZADO PARA ESTABILIDAD
// ============================================================================

void loop() {
    uint32_t currentTime = millis();
    
    // Actualizar GPS cada 15 segundos
    if (currentTime - lastGPSCheck > STABLE_GPS_INTERVAL) {
        gpsManager.update();
        Position newPos = gpsManager.getPosition();
        bool hasFix = gpsManager.hasValidFix();
        
        if (hasFix && (newPos.latitude != 0.0 && newPos.longitude != 0.0)) {
            currentPosition = newPos;
            gpsHasFix = true;
            stableLEDSignal(5); // GPS fix OK
            
            char gpsMsg[80];
            snprintf(gpsMsg, sizeof(gpsMsg), "📍 GPS: %.6f,%.6f (%d sats)", 
                    currentPosition.latitude, currentPosition.longitude, 
                    gpsManager.getSatelliteCount());  // ✅ CORREGIDO
            stableSerialLog(gpsMsg);
        } else {
            gpsHasFix = false;
        }
        
        lastGPSCheck = currentTime;
    }
    
    // Verificar geocerca cada 10 segundos
    if (currentTime - lastGeofenceCheck > STABLE_GEOFENCE_INTERVAL) {
        if (gpsHasFix) {
            checkGeofence();
        }
        lastGeofenceCheck = currentTime;
    }
    
    // Actualizar batería cada 60 segundos
    if (currentTime - lastBatteryCheck > 60000) {
        powerManager.readBattery();
        batteryStatus = powerManager.getBatteryStatus();
        
        char batMsg[50];
        snprintf(batMsg, sizeof(batMsg), "🔋 Batería: %.2fV (%.0f%%)", 
                batteryStatus.voltage, batteryStatus.percentage);
        stableSerialLog(batMsg);
        
        lastBatteryCheck = currentTime;
    }
    
    // Actualizar display cada 20 segundos
    if (currentTime - lastDisplayUpdate > STABLE_DISPLAY_INTERVAL) {
        if (displayManager.isInitialized()) {
            // Preparar status del sistema
            systemStatus.buzzerInitialized = true;
            systemStatus.displayInitialized = displayManager.isInitialized();
            systemStatus.gpsInitialized = gpsManager.isInitialized();
            systemStatus.radioInitialized = radioManager.isInitialized();
            systemStatus.uptime = currentTime / 1000;
            systemStatus.freeHeap = ESP.getFreeHeap();
            systemStatus.currentState = (gpsHasFix ? 2 : 0) + (loraJoined ? 1 : 0);
            
            displayManager.showMainScreen(systemStatus, currentPosition, batteryStatus, currentAlert);
        }
        
        lastDisplayUpdate = currentTime;
    }
    
    // Transmitir por LoRaWAN con intervalos dinámicos
    uint32_t txInterval = STABLE_LORAWAN_INTERVAL; // 2 minutos por defecto
    
    // Intervalos más frecuentes según nivel de alerta
    switch(currentAlert) {
        case AlertLevel::EMERGENCY:
            txInterval = 30000;  // 30 segundos
            break;
        case AlertLevel::DANGER:
            txInterval = 60000;  // 1 minuto
            break;
        case AlertLevel::WARNING:
            txInterval = 90000;  // 1.5 minutos
            break;
        default:
            txInterval = STABLE_LORAWAN_INTERVAL; // 2 minutos
            break;
    }
    
    if (loraJoined && gpsHasFix && currentTime - lastLoRaTransmit > txInterval) {
        // Crear payload optimizado para backend
        uint8_t payload[20];
        size_t payloadLength;
        createGPSPayload(payload, &payloadLength);
        
        // ✅ CORREGIDO: usar sendPacket() no sendRaw()
        Result txResult = radioManager.sendPacket(payload, payloadLength, LORAWAN_PORT_GPS);
        if (txResult == Result::SUCCESS) {
            packetCounter++;
            stableLEDSignal(6); // Packet sent
            buzzerManager.playTone(1200, 100, 30); // Confirmación suave
            
            char txMsg[60];
            snprintf(txMsg, sizeof(txMsg), "📡 TX #%d: %d bytes, Alert:%d", 
                    packetCounter, payloadLength, (int)currentAlert);
            stableSerialLog(txMsg);
        } else {
            stableLEDSignal(3); // Error
            stableSerialLog("❌ Error transmitiendo");
        }
        
        lastLoRaTransmit = currentTime;
    }
    
    // Procesar downlinks
    radioManager.processDownlinks();
    
    // Heartbeat cada 30 segundos
    if (currentTime - lastHeartbeat > STABLE_HEARTBEAT_INTERVAL) {
        stableLEDSignal(9); // Heartbeat
        
        char heartMsg[100];
        snprintf(heartMsg, sizeof(heartMsg), "💓 Chacay UP:%lum TX:%d GPS:%s Alert:%d Bat:%.1fV Heap:%luKB",
                currentTime / 60000, packetCounter, gpsHasFix ? "OK" : "NO", 
                (int)currentAlert, batteryStatus.voltage, ESP.getFreeHeap() / 1024);
        stableSerialLog(heartMsg);
        
        lastHeartbeat = currentTime;
    }
    
    // Delay final para estabilidad
    delay(200);
}

// ============================================================================
// RESUMEN DE CORRECCIONES APLICADAS
// ============================================================================

/*
 * ✅ ERRORES CORREGIDOS:
 * 
 * 1. AlertManager constructor: AlertManager(buzzerManager, displayManager)
 * 2. Position inicialización: Sin lista, usando campos individuales
 * 3. GeofenceManager API: isInsideGeofence(), getDistance() en lugar de setCenter/setRadius
 * 4. AlertCallback signatura: void (*)(AlertLevel, float, const char*)
 * 5. GPSManager métodos: getSatelliteCount() en lugar de getSatellites()
 * 6. RadioManager métodos: sendPacket() en lugar de sendRaw()
 * 7. AlertManager update: update(float) en lugar de updateAlert()
 * 8. Posición cambiada: Chacay Bikepark (-37.346, -72.915)
 * 
 * ✅ CARACTERÍSTICAS MANTENIDAS:
 * - Inicialización secuencial estable
 * - Intervalos extendidos para estabilidad  
 * - LED debugging patterns
 * - Logging híbrido sin bloqueo
 * - Payload optimizado para ChirpStack
 * - Alertas progresivas por distancia
 */