#include <Arduino.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include <Preferences.h>

// Configuración del sistema
#include "config/pins.h"
#include "config/constants.h"
#include "config/lorawan_config.h"
#include "core/Types.h"
#include "core/Logger.h"

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
// MANAGERS (ORDEN CORRECTO DE INICIALIZACIÓN)
// ============================================================================
BuzzerManager buzzerManager(BUZZER_PIN);
PowerManager powerManager(VBAT_PIN);
DisplayManager displayManager;
GPSManager gpsManager;
RadioManager radioManager;
GeofenceManager geofenceManager;
AlertManager alertManager(buzzerManager, displayManager);

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================
#define BUTTON_PRG_PIN  0

struct ButtonState {
    bool lastState = HIGH;
    uint32_t lastDebounceTime = 0;
    const uint32_t debounceDelay = 50;
} buttonState;

uint8_t currentScreen = 0;
const uint8_t TOTAL_SCREENS = 4;

Position currentPosition;
BatteryStatus batteryStatus;
SystemStatus systemStatus;

uint32_t lastGPSCheck = 0;
uint32_t lastBatteryCheck = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastLoRaTransmit = 0;
uint32_t lastGeofenceCheck = 0;

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
    // ... (Tu código de onGeofenceUpdate aquí si lo tienes)
}

// ============================================================================
// PAYLOAD PARA CHIRPSTACK
// ============================================================================
void createDeviceStatusPayloadV2(uint8_t* buffer, size_t* length) {
    static uint8_t frameCounter = 0;
    frameCounter++;
    
    Geofence currentGeofenceData = geofenceManager.getGeofence();
    bool insideGeofence = geofenceManager.isInsideGeofence(currentPosition);
    
    GPSPayloadV2 payloadV2;
    createDeviceStatusPayload(&payloadV2, currentPosition, batteryStatus, 
                             currentAlert, currentGeofenceData, gpsHasFix, 
                             insideGeofence, frameCounter);
    
    memcpy(buffer, &payloadV2, sizeof(payloadV2));
    *length = sizeof(payloadV2);
}

// ============================================================================
// MANEJO DEL BOTÓN
// ============================================================================
void handleButtonPress() {
    bool currentState = digitalRead(BUTTON_PRG_PIN);
    if (currentState == LOW && buttonState.lastState == HIGH && millis() - buttonState.lastDebounceTime > buttonState.debounceDelay) {
        currentScreen = (currentScreen + 1) % TOTAL_SCREENS;
        LOG_I("📺 Cambiando a pantalla: %d", currentScreen);
        buzzerManager.playTone(1200, 50, 60);
        buttonState.lastDebounceTime = millis();
    }
    buttonState.lastState = currentState;
}

// ============================================================================
// SETUP - RÁPIDO Y NO BLOQUEANTE
// ============================================================================
void setup() {
    // Inicializar Serial al principio de todo para asegurar la comunicación
    Serial.begin(115200);
    // Pausa crucial para que el monitor serie del ESP32-S3 se estabilice
    delay(2500);

    LOG_I("=============================================");
    LOG_I("🚀 INICIANDO COLLAR GEOFENCING v3.0");
    LOG_I("=============================================");

    // Configuración básica de hardware
    pinMode(VEXT_ENABLE, OUTPUT);
    digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    randomSeed(analogRead(A0) + millis());
    pinMode(BUTTON_PRG_PIN, INPUT_PULLUP);

    // Inicializar I2C
    Wire.begin(OLED_SDA, OLED_SCL);

    // Inicialización secuencial de Managers
    buzzerManager.init();
    powerManager.init();
    displayManager.init();
    gpsManager.init();
    geofenceManager.init();
    alertManager.init();
    
    buzzerManager.playStartupMelody();
    displayManager.showSplashScreen();
    delay(2000);

    // Configurar RadioManager, pero SIN intentar el join aquí
    if (radioManager.init() == Result::SUCCESS) {
        radioManager.setGeofenceUpdateCallback(onGeofenceUpdate);
        
        if (radioManager.setupLoRaWAN() == Result::SUCCESS) {
            LOG_I("📡 RadioManager listo para intentar Join desde el loop.");
        } else {
            LOG_E("❌ Fallo en setupLoRaWAN.");
        }
    } else {
        LOG_E("❌ Fallo en radioManager.init().");
    }

    systemReady = true;
    LOG_I("✅ Setup completado. Iniciando loop principal.");
}

// ============================================================================
// LOOP - TAREAS LARGAS SE MANEJAN AQUÍ
// ============================================================================
void loop() {
    uint32_t currentTime = millis();

    // --- LÓGICA DE CONEXIÓN LORAWAN NO BLOQUEANTE ---
    if (!loraJoined) {
        static uint32_t lastJoinAttempt = 0;
        // Intentar conectarse cada 30 segundos si no lo ha logrado
        if (currentTime - lastJoinAttempt > 30000) {
            LOG_I("📡 Intentando OTAA Join...");
            if (radioManager.joinOTAA(LORAWAN_DEV_EUI, LORAWAN_APP_EUI, LORAWAN_APP_KEY) == Result::SUCCESS) {
                loraJoined = true; // ¡Conectado!
                buzzerManager.playSuccessTone();
            } else {
                LOG_E("❌ Falló el intento de Join. Reintentando más tarde...");
                buzzerManager.playErrorTone();
            }
            lastJoinAttempt = currentTime;
        }
    }

    // --- MANEJO DEL BOTÓN ---
    handleButtonPress();

    // --- ACTUALIZACIÓN DE SENSORES Y ESTADOS (con intervalos) ---
    if (currentTime - lastGPSCheck > 5000) { // Cada 5 segundos
        gpsManager.update();
        if (gpsManager.hasValidFix()) {
            if (!gpsHasFix) { // Si acabamos de obtener el fix
                LOG_I("🛰️ GPS Fix Obtenido!");
                buzzerManager.playTone(1500, 100);
            }
            currentPosition = gpsManager.getPosition();
            gpsHasFix = true;
        } else {
            gpsHasFix = false;
        }
        lastGPSCheck = currentTime;
    }

    if (currentTime - lastBatteryCheck > 60000) { // Cada 1 minuto
        powerManager.readBattery();
        batteryStatus = powerManager.getBatteryStatus();
        LOG_BATTERY(batteryStatus.voltage, batteryStatus.percentage);
        lastBatteryCheck = currentTime;
    }

    if (currentTime - lastGeofenceCheck > 10000) { // Cada 10 segundos
        if (gpsHasFix && geofenceManager.isActive()) {
            // Lógica para revisar la geocerca y actualizar el nivel de alerta
            // alertManager.update(geofenceManager.getDistance(currentPosition));
        }
        lastGeofenceCheck = currentTime;
    }

    // --- TRANSMISIÓN LORAWAN ---
    uint32_t txInterval = (currentAlert > AlertLevel::SAFE) ? TX_INTERVAL_ALERT : TX_INTERVAL_NORMAL;
    if (loraJoined && gpsHasFix && (currentTime - lastLoRaTransmit > txInterval)) {
        uint8_t payload[20];
        size_t payloadLength;
        createDeviceStatusPayloadV2(payload, &payloadLength);
        
        LOG_I("📡 Enviando uplink LoRaWAN...");
        Result txResult = radioManager.sendPacket(payload, payloadLength, LORAWAN_PORT_GPS);
        if (txResult == Result::SUCCESS) {
            packetCounter++;
            LOG_I("📡 Uplink #%d enviado.", packetCounter);
        } else {
            LOG_E("❌ Falló el envío del uplink.");
        }
        lastLoRaTransmit = currentTime;
    }

    // --- ACTUALIZACIÓN DE PANTALLA ---
    if (currentTime - lastDisplayUpdate > 2000) { // Cada 2 segundos
        systemStatus.uptime = millis();
        systemStatus.freeHeap = ESP.getFreeHeap();
        
        switch (currentScreen) {
            case 0:
                displayManager.showMainScreen(systemStatus, currentPosition, batteryStatus, currentAlert);
                break;
            case 1:
                displayManager.showGPSDetailScreen(currentPosition);
                break;
            case 2:
                {
                    Geofence gf = geofenceManager.getGeofence();
                    float dist = geofenceManager.getDistance(currentPosition);
                    bool inside = geofenceManager.isInsideGeofence(currentPosition);
                    displayManager.showGeofenceInfoScreen(gf, dist, inside);
                }
                break;
            case 3:
                {
                    SystemStats stats; // Llenar con datos reales
                    stats.totalPacketsSent = packetCounter;
                    displayManager.showSystemStatsScreen(stats);
                }
                break;
        }
        lastDisplayUpdate = currentTime;
    }
    
    // --- TAREAS CONTINUAS ---
    radioManager.processDownlinks();
    alertManager.update();
    
    // Pequeño delay para ceder tiempo al sistema operativo
    delay(10);
}