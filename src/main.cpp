/**
 * ============================================================================
 * COLLAR BUENACABRA V3.0 - MAIN
 * ============================================================================
 * Sistema de geofencing con LoRaWAN para ganado
 * Hardware: Heltec WiFi LoRa 32 V3
 *
 * @author BuenaCabra Team
 * @version 3.0.0
 * @date 2025
 */

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>

// ============================================================================
// CONFIGURACIÓN DE PINES (HELTEC V3) - TEMPORAL HASTA QUE CARGUEN LOS HEADERS
// ============================================================================
#ifndef LED_PIN
#define LED_PIN 35
#endif
#ifndef PRG_BUTTON
#define PRG_BUTTON 0
#endif
#ifndef VEXT_ENABLE
#define VEXT_ENABLE 36
#endif
#ifndef VEXT_ON_VALUE
#define VEXT_ON_VALUE LOW // LOW activa la alimentación en Heltec V3
#endif
#ifndef SERIAL_BAUD
#define SERIAL_BAUD 115200
#endif
#ifndef BUZZER_PIN
#define BUZZER_PIN 7
#endif
#ifndef VBAT_PIN
#define VBAT_PIN 1
#endif
#ifndef OLED_SDA
#define OLED_SDA 17
#endif
#ifndef OLED_SCL
#define OLED_SCL 18
#endif

// ============================================================================
// MANAGERS (incluir después de definir pines)
// ============================================================================
// Core includes
#include "core/Logger.h"

// Hardware managers
#include "hardware/BuzzerManager.h"
#include "hardware/PowerManager.h"
#include "hardware/DisplayManager.h"
#include "hardware/GPSManager.h"
#include "hardware/RadioManager.h"

// System managers
#include "system/GeofenceManager.h"
#include "system/AlertManager.h"

// ============================================================================
// INSTANCIAS GLOBALES
// ============================================================================
BuzzerManager buzzerManager(BUZZER_PIN);
PowerManager powerManager(VBAT_PIN);
DisplayManager displayManager;
GPSManager gpsManager;
RadioManager radioManager;
GeofenceManager geofenceManager;
AlertManager alertManager(buzzerManager, displayManager);

// ============================================================================
// VARIABLES DE ESTADO
// ============================================================================
enum SystemState
{
    STATE_INIT,
    STATE_WAITING_JOIN,
    STATE_OPERATIONAL,
    STATE_ERROR
};

SystemState systemState = SystemState::STATE_INIT;

// Estructuras de datos
Position currentPosition;
BatteryStatus batteryStatus;
SystemStatus systemStatus;

// Timers
uint32_t lastGPSUpdate = 0;
uint32_t lastBatteryCheck = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastLoRaTransmit = 0;
uint32_t lastHeartbeat = 0;
uint32_t lastSerialStatus = 0;

// Estados
bool loraJoined = false;
bool gpsHasFix = false;
uint16_t packetCounter = 0;
uint8_t currentScreen = 0;
const uint8_t TOTAL_SCREENS = 4;

// Configuración de tiempos (ms) - ya definidos en constants.h

// ============================================================================
// FUNCIONES DE UTILIDAD
// ============================================================================

void blinkLED(uint8_t times, uint16_t delayMs = 100)
{
    for (uint8_t i = 0; i < times; i++)
    {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
}

void handleButton()
{
    static bool lastButtonState = HIGH;
    static uint32_t lastDebounceTime = 0;
    const uint32_t debounceDelay = 50;

    bool reading = digitalRead(PRG_BUTTON);

    if (reading != lastButtonState)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        if (reading == LOW)
        {
            currentScreen = (currentScreen + 1) % TOTAL_SCREENS;
            Serial.print(F("📺 Pantalla cambiada a: "));
            Serial.println(currentScreen);
            buzzerManager.playTone(1200, 50, 60);
        }
    }

    lastButtonState = reading;
}

// ============================================================================
// CALLBACK PARA GEOCERCA
// ============================================================================
void onGeofenceUpdate(const GeofenceUpdate &update)
{
    Serial.print(F("🌐 Geocerca actualizada: "));
    Serial.println(update.name);
    Serial.print(F("   • Centro: "));
    Serial.print(update.centerLat, 6);
    Serial.print(F(", "));
    Serial.println(update.centerLng, 6);
    Serial.print(F("   • Radio: "));
    Serial.print(update.radius);
    Serial.println(F(" metros"));

    // Guardar la geocerca
    geofenceManager.setGeofence(update.centerLat, update.centerLng, update.radius, update.name);

    // Feedback visual y sonoro
    blinkLED(3, 200);
    buzzerManager.playTone(1500, 100, 100);
    delay(100);
    buzzerManager.playTone(2000, 100, 100);
}

// ============================================================================
// FUNCIONES DE INICIALIZACIÓN
// ============================================================================

bool initHardware()
{
    LOG_I("\n🔧 INICIALIZANDO HARDWARE...");

    // Configurar pines básicos
    pinMode(LED_PIN, OUTPUT);
    pinMode(PRG_BUTTON, INPUT_PULLUP);
    pinMode(VEXT_ENABLE, OUTPUT);

    // IMPORTANTE: Activar alimentación de periféricos (LOW = ON en Heltec V3)
    digitalWrite(VEXT_ENABLE, LOW); // LOW activa VEXT
    digitalWrite(LED_PIN, LOW);

    // Esperar a que se estabilice la alimentación
    delay(500); // Aumentado para dar tiempo al display y GPS

    LOG_I("   ✓ Pines configurados y VEXT activado");

    // Inicializar I2C
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000); // 400kHz para mejor velocidad
    LOG_I("   ✓ I2C inicializado");

    delay(100); // Dar tiempo adicional
    return true;
}

bool initManagers()
{
    LOG_I("\n🚀 INICIALIZANDO MANAGERS...");
    bool allOk = true;

    // Power Manager
    if (powerManager.init() == Result::SUCCESS)
    {
        LOG_I("   ✓ Power Manager OK");
    }
    else
    {
        LOG_E("   ✗ Power Manager FALLÓ");
        allOk = false;
    }

    // Buzzer Manager
    if (buzzerManager.init() == Result::SUCCESS)
    {
        LOG_I("   ✓ Buzzer Manager OK");
        buzzerManager.playTone(1000, 50, 50); // Beep de confirmación
    }
    else
    {
        LOG_E("   ✗ Buzzer Manager FALLÓ");
        allOk = false;
    }

    // Display Manager
    if (displayManager.init() == Result::SUCCESS)
    {
        LOG_I("   ✓ Display Manager OK");
        displayManager.showSplashScreen();
    }
    else
    {
        LOG_E("   ✗ Display Manager FALLÓ");
        allOk = false;
    }

    // GPS Manager
    if (gpsManager.init() == Result::SUCCESS)
    {
        LOG_I("   ✓ GPS Manager OK");
    }
    else
    {
        LOG_E("   ✗ GPS Manager FALLÓ");
        allOk = false;
    }

    // Geofence Manager
    if (geofenceManager.init() == Result::SUCCESS)
    {
        LOG_I("   ✓ Geofence Manager OK");
        Geofence gf = geofenceManager.getGeofence();
        if (gf.isConfigured)
        {
            LOG_I("     → Geocerca cargada: ");
            LOG_I(gf.name);
        }
        else
        {
            LOG_I("     → Sin geocerca configurada");
        }
    }
    else
    {
        LOG_E("   ✗ Geofence Manager FALLÓ");
        allOk = false;
    }

    // Alert Manager
    if (alertManager.init() == Result::SUCCESS)
    {
        LOG_I("   ✓ Alert Manager OK");
    }
    else
    {
        LOG_E("   ✗ Alert Manager FALLÓ");
        allOk = false;
    }

    // Radio Manager (más complejo)
    LOG_I("   🔄 Inicializando Radio...");
    if (radioManager.init() == Result::SUCCESS)
    {
        LOG_I("   ✓ Radio inicializada");

        if (radioManager.setupLoRaWAN() == Result::SUCCESS)
        {
            LOG_I("   ✓ LoRaWAN configurado");
            radioManager.setGeofenceUpdateCallback(onGeofenceUpdate);
        }
        else
        {
            LOG_E("   ✗ LoRaWAN configuración FALLÓ");
            allOk = false;
        }
    }
    else
    {
        LOG_E("   ✗ Radio inicialización FALLÓ");
        allOk = false;
    }

    return allOk;
}

// ============================================================================
// FUNCIONES DE OPERACIÓN
// ============================================================================

void updateGPS()
{
    gpsManager.update();

    if (gpsManager.hasValidFix())
    {
        if (!gpsHasFix)
        {
            LOG_I("🛰️ GPS FIX OBTENIDO!");
            blinkLED(2, 100);
        }
        gpsHasFix = true;
        currentPosition = gpsManager.getPosition();

        // Log ocasional de posición
        static uint32_t lastGPSLog = 0;
        if (millis() - lastGPSLog > 30000)
        {
            LOG_I("📍 Posición: %.6f, %.6f | Sats: %d",
                  currentPosition.latitude,
                  currentPosition.longitude,
                  gpsManager.getSatelliteCount());
            lastGPSLog = millis();
        }
    }
    else
    {
        if (gpsHasFix)
        {
            LOG_W("⚠️ GPS FIX PERDIDO");
        }
        gpsHasFix = false;

        // Mostrar satélites visibles aunque no haya fix
        static uint32_t lastSatLog = 0;
        if (millis() - lastSatLog > 10000)
        { // Log cada 10 segundos
            uint8_t sats = gpsManager.getSatelliteCount();
            if (sats > 0)
            {
                LOG_I("🛰️ Satélites visibles: ");
                LOG_I("%d", sats);
            }
            lastSatLog = millis();
        }
    }
}

void sendLoRaPacket()
{
    if (!loraJoined || !gpsHasFix)
        return;

    // Preparar payload
    uint8_t payload[32];
    size_t payloadLength = 0;

    // Crear payload simplificado
    // [0] = Tipo de mensaje (0x01 = posición)
    payload[0] = 0x01;

    // [1-4] = Latitud (float)
    memcpy(&payload[1], &currentPosition.latitude, 4);

    // [5-8] = Longitud (float)
    memcpy(&payload[5], &currentPosition.longitude, 4);

    // [9] = Batería (%)
    payload[9] = (uint8_t)batteryStatus.percentage;

    // [10] = Estado de alerta
    Geofence gf = geofenceManager.getGeofence();
    bool insideGeofence = geofenceManager.isInsideGeofence(currentPosition);
    payload[10] = insideGeofence ? 0x00 : 0x01;

    // [11] = Número de satélites
    payload[11] = gpsManager.getSatelliteCount();

    payloadLength = 12;

    // Enviar
    if (radioManager.sendPacket(payload, payloadLength, LORAWAN_PORT_GPS) == Result::SUCCESS)
    {
        packetCounter++;
        Serial.print(F("📡 Uplink #"));
        Serial.print(packetCounter);
        Serial.println(F(" enviado"));
        blinkLED(1, 50);
    }
    else
    {
        Serial.println(F("❌ Error enviando uplink"));
    }
}

void updateDisplay()
{
    // Actualizar información del sistema
    systemStatus.buzzerInitialized = buzzerManager.isInitialized();
    systemStatus.displayInitialized = displayManager.isInitialized();
    systemStatus.gpsInitialized = gpsManager.isInitialized();
    systemStatus.radioInitialized = radioManager.isInitialized();
    systemStatus.uptime = millis() / 1000;
    systemStatus.freeHeap = ESP.getFreeHeap();
    systemStatus.currentState = systemState;

    // Actualizar batería
    powerManager.readBattery();
    batteryStatus = powerManager.getBatteryStatus();

    // Determinar nivel de alerta
    AlertLevel alertLevel = AlertLevel::SAFE;
    if (geofenceManager.getGeofence().isConfigured)
    {
        float distance = geofenceManager.getDistance(currentPosition);
        bool inside = geofenceManager.isInsideGeofence(currentPosition);

        if (!inside)
        {
            if (distance > 500)
                alertLevel = AlertLevel::EMERGENCY;
            else if (distance > 300)
                alertLevel = AlertLevel::DANGER;
            else if (distance > 150)
                alertLevel = AlertLevel::WARNING;
            else
                alertLevel = AlertLevel::CAUTION;
        }
    }

    // Mostrar pantalla según selección
    switch (currentScreen)
    {
    case 0:
        displayManager.showMainScreen(systemStatus, currentPosition, batteryStatus, alertLevel);
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
        SystemStats stats;
        stats.totalPacketsSent = packetCounter;
        stats.successfulPackets = packetCounter; // Por ahora asumimos todos exitosos
        stats.failedPackets = 0;
        stats.lastRSSI = -80; // Placeholder
        stats.lastSNR = 5.0;  // Placeholder
        displayManager.showSystemStatsScreen(stats);
    }
    break;
    }
}

void printSerialStatus()
{
    Serial.println(F("\n📊 ESTADO DEL SISTEMA:"));
    Serial.print(F("   • Estado: "));
    switch (systemState)
    {
    case STATE_INIT:
        Serial.println(F("INICIALIZANDO"));
        break;
    case STATE_WAITING_JOIN:
        Serial.println(F("ESPERANDO JOIN"));
        break;
    case STATE_OPERATIONAL:
        Serial.println(F("OPERACIONAL"));
        break;
    case STATE_ERROR:
        Serial.println(F("ERROR"));
        break;
    }
    Serial.print(F("   • LoRa: "));
    Serial.println(loraJoined ? F("CONECTADO") : F("DESCONECTADO"));
    Serial.print(F("   • GPS: "));
    Serial.println(gpsHasFix ? F("FIX OK") : F("SIN FIX"));
    Serial.print(F("   • Batería: "));
    Serial.print(batteryStatus.voltage);
    Serial.print(F("V ("));
    Serial.print(batteryStatus.percentage);
    Serial.println(F("%)"));
    Serial.print(F("   • Paquetes enviados: "));
    Serial.println(packetCounter);
    Serial.print(F("   • Uptime: "));
    Serial.print(millis() / 1000);
    Serial.println(F(" segundos"));
    Serial.print(F("   • Memoria libre: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));

    Geofence gf = geofenceManager.getGeofence();
    if (gf.isConfigured)
    {
        Serial.print(F("   • Geocerca: "));
        Serial.println(gf.name);
        if (gpsHasFix)
        {
            float dist = geofenceManager.getDistance(currentPosition);
            bool inside = geofenceManager.isInsideGeofence(currentPosition);
            Serial.print(F("     → Distancia: "));
            Serial.print(dist);
            Serial.println(F(" m"));
            Serial.print(F("     → Estado: "));
            Serial.println(inside ? F("DENTRO") : F("FUERA"));
        }
    }
    else
    {
        Serial.println(F("   • Geocerca: NO CONFIGURADA"));
    }
}

// ============================================================================
// CONFIGURACIÓN DE LORAWAN
// ============================================================================
// IMPORTANTE: Reemplazar con tus propias claves de ChirpStack
const uint8_t LORAWAN_DEV_EUI[8] = {0x58, 0xEC, 0x3C, 0x43, 0xCA, 0x48, 0x00, 0x00};
const uint8_t LORAWAN_APP_EUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t LORAWAN_APP_KEY[16] = {
    0x12, 0x8A, 0x9F, 0x0C, 0x8B, 0x8E, 0xFB, 0x6D,
    0xCD, 0x33, 0xC2, 0x37, 0x06, 0x27, 0x2E, 0x75};

// ============================================================================
// SETUP
// ============================================================================
void setup()
{
    // Inicializar Serial primero. Lo mantenemos por LEGACY
    Serial.begin(SERIAL_BAUD);

    // Inicializar logger.
    Logger::init(SERIAL_BAUD);

    // Mostrar información del sistema
    Logger::printSystemInfo();

    // Inicializar hardware básico
    if (!initHardware())
    {
        Serial.println(F("❌ ERROR CRÍTICO: Hardware básico falló"));
        systemState = STATE_ERROR;
        return;
    }

    // Inicializar managers
    if (!initManagers())
    {
        Serial.println(F("⚠️ ADVERTENCIA: Algunos managers fallaron"));
        // Continuar de todos modos, algunos componentes pueden funcionar
    }

    // Melodía de inicio
    if (buzzerManager.isInitialized())
    {
        buzzerManager.playStartupMelody();
    }

    // LED indica inicio exitoso
    blinkLED(3, 200);

    Serial.println(F("\n✅ SISTEMA INICIADO - ENTRANDO EN MODO OPERACIONAL\n"));
    LOG_I("EL LOGGER ROBUSTO FUNCIONA!!!");
    systemState = STATE_WAITING_JOIN;

    // NUEVO  para persistir sesion: Verificar si ya tenemos sesión LoRaWAN válida
    if (radioManager.isJoined())
    {
        Serial.println(F("🔄 Sesión LoRaWAN restaurada desde memoria"));
        loraJoined = true;
        systemState = STATE_OPERATIONAL;
        blinkLED(2, 300); // LED diferente para sesión restaurada
    }

    // Inicializar timers
    lastGPSUpdate = millis();
    lastBatteryCheck = millis();
    lastDisplayUpdate = millis();
    lastLoRaTransmit = millis();
    lastHeartbeat = millis();
    lastSerialStatus = millis();
}

// ============================================================================
// LOOP
// ============================================================================
void loop()
{
    uint32_t now = millis();

    // Manejo de estado de error
    if (systemState == STATE_ERROR)
    {
        static uint32_t lastErrorBlink = 0;
        if (now - lastErrorBlink > 1000)
        {
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
            lastErrorBlink = now;
        }
        delay(10);
        return;
    }

    // Heartbeat LED
    if (now - lastHeartbeat > HEARTBEAT_INTERVAL)
    {
        blinkLED(1, 50);
        lastHeartbeat = now;
    }

    // Join LoRaWAN si no está conectado
    if (systemState == STATE_WAITING_JOIN && !loraJoined)
    {
        static uint32_t lastJoinAttempt = 0;
        static uint8_t joinAttempts = 0;
        const uint8_t MAX_JOIN_ATTEMPTS = 5;

        if (now - lastJoinAttempt > 30000)
        { // Intentar cada 30 segundos
            joinAttempts++;
            Serial.print(F("\n📡 Intento JOIN #"));
            Serial.print(joinAttempts);
            Serial.println(F(" LoRaWAN..."));

            if (radioManager.joinOTAA(LORAWAN_DEV_EUI, LORAWAN_APP_EUI, LORAWAN_APP_KEY) == Result::SUCCESS)
            {
                Serial.println(F("✅ JOIN EXITOSO!"));
                loraJoined = true;
                systemState = STATE_OPERATIONAL;
                joinAttempts = 0; // Reset contador
                blinkLED(5, 100);
                if (buzzerManager.isInitialized())
                {
                    buzzerManager.playTone(2000, 200, 200);
                }
            }
            else
            {
                Serial.print(F("❌ JOIN FALLÓ - Intento "));
                Serial.print(joinAttempts);
                Serial.print(F("/"));
                Serial.println(MAX_JOIN_ATTEMPTS);

                // Si hemos fallado muchas veces, limpiar todo y reiniciar
                if (joinAttempts >= MAX_JOIN_ATTEMPTS)
                {
                    Serial.println(F("🔄 Demasiados fallos de JOIN, limpiando sesión y reiniciando..."));
                    radioManager.forceRejoin();
                    delay(2000);
                    ESP.restart();
                }
            }
            lastJoinAttempt = now;
        }
    }

    // Actualizar GPS
    if (now - lastGPSUpdate > GPS_UPDATE_INTERVAL)
    {
        updateGPS();
        lastGPSUpdate = now;
    }

    // Verificar batería
    if (now - lastBatteryCheck > BATTERY_CHECK_INTERVAL)
    {
        powerManager.readBattery();
        batteryStatus = powerManager.getBatteryStatus();

        if (batteryStatus.percentage < 20)
        {
            Serial.println(F("⚠️ BATERÍA BAJA!"));
            if (buzzerManager.isInitialized())
            {
                buzzerManager.playTone(500, 100, 100);
            }
        }
        lastBatteryCheck = now;
    }

    // Enviar datos por LoRa
    if (systemState == STATE_OPERATIONAL && (now - lastLoRaTransmit > LORA_TX_INTERVAL))
    {
        sendLoRaPacket();
        lastLoRaTransmit = now;
    }

    // Actualizar display
    if (now - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL)
    {
        updateDisplay();
        lastDisplayUpdate = now;
    }

    // Imprimir estado en Serial
    if (now - lastSerialStatus > SERIAL_STATUS_INTERVAL)
    {
        printSerialStatus();
        lastSerialStatus = now;
    }

    // Procesar downlinks
    if (radioManager.isInitialized())
    {
        radioManager.processDownlinks();
    }

    // Manejar botón
    handleButton();

    // Verificar geocerca si está configurada
    if (geofenceManager.getGeofence().isConfigured && gpsHasFix)
    {
        static uint32_t lastGeofenceCheck = 0;
        if (now - lastGeofenceCheck > 10000)
        { // Cada 10 segundos
            bool inside = geofenceManager.isInsideGeofence(currentPosition);
            float distance = geofenceManager.getDistance(currentPosition);

            if (!inside && distance > 100)
            {
                static uint32_t lastAlert = 0;
                if (now - lastAlert > 60000)
                { // Alerta cada minuto máximo
                    Serial.print(F("🚨 ALERTA: Fuera de geocerca! Distancia: "));
                    Serial.print(distance);
                    Serial.println(F(" metros"));

                    if (buzzerManager.isInitialized())
                    {
                        buzzerManager.playAlertTone(AlertLevel::WARNING);
                    }
                    lastAlert = now;
                }
            }
            lastGeofenceCheck = now;
        }
    }

    // Pequeño delay para no saturar el CPU
    delay(10);
}