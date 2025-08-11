
#include <Arduino.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include <Preferences.h>  // Para persistencia de DevNonce y Frame Counters

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
#define STABLE_GPS_INTERVAL         15000   // 15s
#define STABLE_DISPLAY_INTERVAL     20000   // 20s  
#define STABLE_GEOFENCE_INTERVAL    10000   // 10s
#define STABLE_LORAWAN_INTERVAL     120000  // 2min
#define STABLE_HEARTBEAT_INTERVAL   30000   // 30s

// ============================================================================
// MANAGERS (ORDEN CORRECTO DE INICIALIZACIÓN)
// ============================================================================

// Hardware managers primero
BuzzerManager buzzerManager(BUZZER_PIN);
PowerManager powerManager(VBAT_PIN);
DisplayManager displayManager;
GPSManager gpsManager;
RadioManager radioManager;
GeofenceManager geofenceManager;

// AlertManager DEBE ir después de buzzer y display (requiere referencias)
AlertManager alertManager(buzzerManager, displayManager);  // ✅ CORREGIDO

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

#define BUTTON_PRG_PIN  0  // GPIO0 es el botón PRG en Heltec V3

// Estado del botón
struct ButtonState {
    bool lastState = HIGH;
    bool currentState = HIGH;
    uint32_t lastDebounceTime = 0;
    const uint32_t debounceDelay = 50;
    uint32_t pressCount = 0;
} buttonState;

// Estado de las pantallas
struct ScreenManager {
    uint8_t currentScreen = 0;
    uint32_t lastScreenUpdate = 0;
    const uint32_t screenUpdateInterval = 1000; // Actualizar cada 1s
    
    // Para rotación automática (opcional)
    bool autoRotate = false;
    uint32_t autoRotateInterval = 10000; // 10s por pantalla
    uint32_t lastAutoRotate = 0;
} screenManager;

// Información de geocerca actual
struct GeofenceInfo {
    String name = "Bosques Chacay";
    GeofenceType type = GeofenceType::CIRCLE;
    float radius = 100.0;
    float currentDistance = 0;
    bool isInside = true;
    double centerLat = -37.34640277978371;
    double centerLng = -72.91495492379738;
} currentGeofence;

// Posición por defecto (sin lista de inicialización)
Position currentPosition;  // ✅ CORREGIDO
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

// ============================================================================
// CALLBACK PARA ACTUALIZACIONES DE GEOCERCA VÍA DOWNLINK
// ============================================================================

void onGeofenceUpdate(const GeofenceUpdate& update) {
    LOG_I("🌐 Actualizando geocerca desde downlink:");
    LOG_I("  Nombre: %s", update.name);
    LOG_I("  Centro: %.6f, %.6f", update.centerLat, update.centerLng);
    LOG_I("  Radio: %.0f metros", update.radius);
    
    // Actualizar GeofenceManager
    geofenceManager.setGeofence(update.centerLat, update.centerLng, update.radius, update.name);
    
    // Actualizar información local para la pantalla
    currentGeofence.name = String(update.name);
    currentGeofence.centerLat = update.centerLat;
    currentGeofence.centerLng = update.centerLng;
    currentGeofence.radius = update.radius;
    currentGeofence.type = GeofenceType::CIRCLE; // Solo círculos por ahora
    
    // Emitir tono de confirmación
    buzzerManager.playTone(1500, 200, 60);  // Tono de confirmación
    delay(100);
    buzzerManager.playTone(1800, 200, 60);  // Tono doble
    
    LOG_I("✅ Geocerca actualizada exitosamente desde LoRaWAN");
    
    // Recalcular distancia actual con la nueva geocerca
    if (gpsHasFix) {
        currentGeofence.currentDistance = geofenceManager.getDistance(currentPosition);
        currentGeofence.isInside = geofenceManager.isInsideGeofence(currentPosition);
        
        LOG_I("📍 Nueva distancia a geocerca: %.1fm (Dentro: %s)", 
              currentGeofence.currentDistance, currentGeofence.isInside ? "Sí" : "No");
    }
}

// ============================================================================
// CONFIGURACIÓN INICIAL
// ============================================================================

void initializeDefaultPosition() {
    // Inicializar posición como inválida hasta obtener GPS real
    currentPosition.latitude = 0.0;
    currentPosition.longitude = 0.0;
    currentPosition.altitude = 0.0f;
    currentPosition.satellites = 0;
    currentPosition.accuracy = 0.0f;
    currentPosition.timestamp = millis();
    currentPosition.valid = false; // Marcar como inválida
}

void setupGeofence() {
    // NO configurar geocerca por defecto al inicio
    // El sistema esperará a recibir una geocerca desde la web o desde memoria
    LOG_I("🌐 Esperando configuración de geocerca desde ChirpStack...");
}

void configureAlertCallbacks() {
    // ✅ CALLBACK CORREGIDO con signatura real: void (*)(AlertLevel, float, const char*)
    alertManager.setAlertCallback([](AlertLevel level, float distance, const char* reason) {
        currentAlert = level;
        
        switch(level) {
            case AlertLevel::SAFE:
                // No hacer nada en SAFE
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
// PAYLOAD PARA CHIRPSTACK
// ============================================================================

struct GPSPayload {
    int32_t latitude;   
    int32_t longitude;    
    uint16_t altitude;  
    uint8_t satellites; 
    uint8_t hdop;       
    uint8_t battery;    
    uint8_t alert;      
    uint8_t status;     
} __attribute__((packed));

void createGPSPayload(uint8_t* buffer, size_t* length) {
    GPSPayload payload;
    
    payload.latitude = (int32_t)(currentPosition.latitude * 10000000);
    payload.longitude = (int32_t)(currentPosition.longitude * 10000000);
    payload.altitude = (uint16_t)currentPosition.altitude;
    
    // ✅ CORREGIDO: getSatelliteCount() en lugar de getSatellites()
    payload.satellites = gpsManager.getSatelliteCount();
    payload.hdop = (uint8_t)(gpsManager.getHDOP() * 10);
    
    payload.battery = (uint8_t)batteryStatus.percentage;
    payload.alert = (uint8_t)currentAlert;
    
    payload.status = 0;
    if (gpsHasFix) payload.status |= (1 << 0);
    // ✅ CORREGIDO: isInsideGeofence() en lugar de isInside()
    if (geofenceManager.isInsideGeofence(currentPosition)) payload.status |= (1 << 1);
    
    memcpy(buffer, &payload, sizeof(payload));
    *length = sizeof(payload);
}

// ============================================================================
// VERIFICACIÓN DE GEOCERCA
// ============================================================================

void checkGeofence() {
    if (!gpsHasFix) return;
    
    // ✅ Usar API real de GeofenceManager
    float distance = geofenceManager.getDistance(currentPosition);
    bool isInside = geofenceManager.isInsideGeofence(currentPosition);  // ✅ CORREGIDO
    
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
        float radius = geofenceManager.getRadius();
        float margin = radius - distance;
        if (margin < 10.0f) {
            newLevel = AlertLevel::CAUTION;
        }
    }
    
    // ✅ CORREGIDO: setAlertLevel() en lugar de updateAlert()
    if (newLevel != currentAlert) {
        alertManager.setAlertLevel(newLevel, distance);
    }
}

// ============================================================================
// SETUP PRINCIPAL
// ============================================================================

// ============================================================================
// FUNCIÓN DE INICIALIZACIÓN DEL BOTÓN
// ============================================================================

void initButton() {
    pinMode(BUTTON_PRG_PIN, INPUT_PULLUP);
    LOG_I("🔘 Botón PRG configurado en pin %d", BUTTON_PRG_PIN);
}

// ============================================================================
// FUNCIÓN PARA LEER EL BOTÓN CON DEBOUNCE
// ============================================================================

bool readButton() {
    int reading = digitalRead(BUTTON_PRG_PIN);
    
    if (reading != buttonState.lastState) {
        buttonState.lastDebounceTime = millis();
    }
    
    if ((millis() - buttonState.lastDebounceTime) > buttonState.debounceDelay) {
        if (reading != buttonState.currentState) {
            buttonState.currentState = reading;
            
            // Detectar presión (flanco descendente)
            if (buttonState.currentState == LOW) {
                buttonState.pressCount++;
                return true;
            }
        }
    }
    
    buttonState.lastState = reading;
    return false;
}

// ============================================================================
// FUNCIÓN PARA MANEJAR CAMBIO DE PANTALLA
// ============================================================================

void handleScreenChange() {
    screenManager.currentScreen = (screenManager.currentScreen + 1) % 4;
    screenManager.lastAutoRotate = millis(); // Reset auto-rotación
    
    // Feedback sonoro solamente
    buzzerManager.playTone(1000, 50, 50); // Beep corto
    
    // Log del cambio
    const char* screenNames[] = {
        "Principal",
        "GPS Detalle",
        "Geocerca",
        "Estadísticas"
    };
    
    LOG_I("📺 Cambiado a pantalla: %s", screenNames[screenManager.currentScreen]);
}

// ============================================================================
// FUNCIÓN PARA ACTUALIZAR PANTALLA SEGÚN MODO
// ============================================================================

void updateDisplay() {
    // No actualizar muy frecuentemente
    if (millis() - screenManager.lastScreenUpdate < screenManager.screenUpdateInterval) {
        return;
    }
    
    screenManager.lastScreenUpdate = millis();
    
    // Actualizar información de geocerca
    if (geofenceManager.isInitialized()) {
        currentGeofence.currentDistance = geofenceManager.getDistance(currentPosition);
        currentGeofence.isInside = geofenceManager.isInsideGeofence(currentPosition);
        
        // Actualizar información en el DisplayManager
        displayManager.updateGeofenceInfo(
            currentGeofence.name.c_str(),
            currentGeofence.type,
            currentGeofence.radius,
            currentGeofence.currentDistance,
            currentGeofence.isInside
        );
    }
    
    // Actualizar contadores
    displayManager.updateCounters(packetCounter, 0); // rxCounter si lo tienes
    
    // Mostrar pantalla según el modo actual
    switch (screenManager.currentScreen) {
        case 0: // Pantalla principal
            displayManager.showMainScreen(systemStatus, currentPosition, 
                                         batteryStatus, currentAlert);
            break;
            
        case 1: // Detalles GPS
            displayManager.showGPSDetailScreen(currentPosition);
            break;
            
        case 2: // Información de Geocerca
            {
                Geofence gf;
                gf.centerLat = currentGeofence.centerLat;
                gf.centerLng = currentGeofence.centerLng;
                gf.radius = currentGeofence.radius;
                strncpy(gf.name, currentGeofence.name.c_str(), 15);
                gf.name[15] = '\0';
                gf.active = true;
                
                displayManager.showGeofenceInfoScreen(gf, 
                    currentGeofence.currentDistance, 
                    currentGeofence.isInside);
            }
            break;
            
        case 3: // Estadísticas del sistema
            {
                SystemStats stats;
                stats.totalUptime = millis();
                stats.totalPacketsSent = packetCounter;
                stats.totalPacketsReceived = 0;
                stats.packetsLost = 0;
                stats.geofenceViolations = 0;
                stats.lowBatteryEvents = 0;
                stats.alertsTriggered = 0;
                stats.averageBatteryVoltage = batteryStatus.voltage;
                
                displayManager.showSystemStatsScreen(stats);
            }
            break;
            
        default:
            screenManager.currentScreen = 0;
            break;
    }
}

void setup() {
    // Configuración básica
    pinMode(VEXT_ENABLE, OUTPUT);
    digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE);
    delay(200);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // ✅ INICIALIZAR RANDOM PARA DEV-NONCE ALEATORIO
    randomSeed(analogRead(A0) + millis() + ESP.getCycleCount());
    
    delay(500);
    
    // Inicializar posición como inválida
    initializeDefaultPosition();
    
    // I2C
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(100000); // Frecuencia conservadora
    
    // 1. BuzzerManager
    if (buzzerManager.init() == Result::SUCCESS) {
        buzzerManager.playTone(1000, 200, 80);
        delay(300);
    }
    delay(500);
    
    // 2. PowerManager
    if (powerManager.init() == Result::SUCCESS) {
        powerManager.readBattery();
        batteryStatus = powerManager.getBatteryStatus();
    }
    delay(500);
    
    // 3. DisplayManager
    if (displayManager.init() == Result::SUCCESS) {
        displayManager.showSplashScreen();
        delay(2000);
    }
    delay(500);
    
    // 4. GPSManager
    gpsManager.init();
    delay(500);
    
    // 5. GeofenceManager
    if (geofenceManager.init() == Result::SUCCESS) {
        setupGeofence();
    }
    delay(500);
    
    // 6. AlertManager
    if (alertManager.init() == Result::SUCCESS) {
        configureAlertCallbacks();
    }
    delay(500);
    
    // 7. RadioManager
    if (radioManager.init() == Result::SUCCESS) {
        // 🔥 CONFIGURAR CALLBACK PARA ACTUALIZACIONES DE GEOCERCA
        radioManager.setGeofenceUpdateCallback(onGeofenceUpdate);
        
        // Configurar LoRa con frecuencia CORRECTA para AU915 Sub-banda 1
        if (radioManager.setupLoRa(916.8, 125.0, 9, 7, 20) == Result::SUCCESS) {
            // Configurar LoRaWAN
            if (radioManager.setupLoRaWAN() == Result::SUCCESS) {
                // ✅ OTAA JOIN CON REINTENTOS
                bool joinAttempted = false;
                for (int attempt = 1; attempt <= 3; attempt++) {
                    if (radioManager.joinOTAA(LORAWAN_DEV_EUI, LORAWAN_APP_EUI, LORAWAN_APP_KEY) == Result::SUCCESS) {
                        loraJoined = true;
                        buzzerManager.playSuccessTone();
                        
                        joinAttempted = true;
                        break; // Éxito, salir del loop
                    } else {
                        delay(5000 * attempt); // Esperar 5s, 10s, 15s entre intentos
                    }
                }
            }
        }
    }
    
    // Inicializar botón PRG
    initButton();
    
    // Configurar pantalla inicial
    screenManager.currentScreen = 0;
    screenManager.autoRotate = false; // Desactivado por defecto
    
    // Sistema listo
    systemReady = true;
    delay(1000);
    
    // Inicializar timers
    uint32_t now = millis();
    lastGPSCheck = now;
    lastBatteryCheck = now;
    lastDisplayUpdate = now;
    lastLoRaTransmit = now;
    lastHeartbeat = now;
    lastGeofenceCheck = now;
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
    uint32_t currentTime = millis();
    
    // === MANEJO DEL BOTÓN PRG ===
    if (readButton()) {
        handleScreenChange();
    }
    
    // Actualizar GPS cada 15 segundos
    if (currentTime - lastGPSCheck > STABLE_GPS_INTERVAL) {
        gpsManager.update();
        Position gpsPos = gpsManager.getPosition();
        bool hasGPSFix = gpsManager.hasValidFix();
        
        if (hasGPSFix && (gpsPos.latitude != 0.0 && gpsPos.longitude != 0.0)) {
            currentPosition = gpsPos;
            gpsHasFix = true;
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
    
    // Actualizar batería cada 30 segundos
    if (currentTime - lastBatteryCheck > 30000) {
        powerManager.readBattery();
        batteryStatus = powerManager.getBatteryStatus();
        
        lastBatteryCheck = currentTime;
    }
    
    // === ACTUALIZACIÓN DE PANTALLA ===
    // Actualizar información del sistema
    systemStatus.buzzerInitialized = true;
    systemStatus.displayInitialized = displayManager.isInitialized();
    systemStatus.gpsInitialized = gpsManager.isInitialized();
    systemStatus.radioInitialized = radioManager.isInitialized();
    systemStatus.uptime = millis();
    systemStatus.freeHeap = ESP.getFreeHeap();
    systemStatus.currentState = (gpsHasFix ? 2 : 0) + (loraJoined ? 1 : 0);
    
    // Navegación normal de pantallas - sin interrupciones de alerta
    updateDisplay();
    
    // ✅ REINTENTAR JOIN SI NO ESTÁ CONECTADO
    static uint32_t lastJoinAttempt = 0;
    if (!loraJoined && currentTime - lastJoinAttempt > 60000) { // Reintentar cada minuto
        if (radioManager.joinOTAA(LORAWAN_DEV_EUI, LORAWAN_APP_EUI, LORAWAN_APP_KEY) == Result::SUCCESS) {
            loraJoined = true;
            buzzerManager.playSuccessTone();
        }
        lastJoinAttempt = currentTime;
    }
    
    // Transmitir por LoRaWAN con intervalos dinámicos
    uint32_t txInterval = STABLE_LORAWAN_INTERVAL; // 2 minutos por defecto
    
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
        uint8_t payload[20];
        size_t payloadLength;
        createGPSPayload(payload, &payloadLength);
        
        // ✅ CORREGIDO: sendPacket() en lugar de sendRaw()
        Result txResult = radioManager.sendPacket(payload, payloadLength, LORAWAN_PORT_GPS);
        if (txResult == Result::SUCCESS) {
            packetCounter++;
            buzzerManager.playTone(1200, 50, 30); // Confirmación suave
        }
        
        lastLoRaTransmit = currentTime;
    }
    
    // 🔥 PROCESAR DOWNLINKS - CRÍTICO PARA ACTUALIZACIONES DE GEOCERCA
    radioManager.processDownlinks();
    
    // Heartbeat cada 30 segundos - simplificado sin LEDs
    if (currentTime - lastHeartbeat > STABLE_HEARTBEAT_INTERVAL) {
        lastHeartbeat = currentTime;
    }
    
    delay(100);
}
