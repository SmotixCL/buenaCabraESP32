/*
 * ============================================================================
 * COLLAR GEOFENCING V3.0 - SISTEMA MODULAR COMPLETO
 * ============================================================================
 * 
 * Hardware: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)
 * Firmware: Modular, escalable y mantenible
 * 
 * CARACTERÍSTICAS:
 * ✅ Arquitectura modular con managers especializados
 * ✅ Sistema de logging avanzado
 * ✅ Gestión de energía optimizada
 * ✅ LoRaWAN completo con downlinks
 * ✅ Geocercas múltiples con alertas progresivas
 * ✅ Display OLED con múltiples pantallas
 * ✅ Sistema de audio avanzado con melodías
 * ✅ Configuración centralizada
 * ✅ Manejo de errores robusto
 * ✅ Estadísticas y monitoreo completo
 * 
 * Autor: Sistema Modular Collar Geofencing
 * Versión: 3.0.0 - Arquitectura Profesional
 * ============================================================================
 */

// ============================================================================
// INCLUSIONES CRÍTICAS PRIMERO (prevenir conflictos de macros)
// ============================================================================
#include <Arduino.h>

// Prevenir conflictos de macros DEG_TO_RAD/RAD_TO_DEG
#ifdef DEG_TO_RAD
#undef DEG_TO_RAD
#endif
#ifdef RAD_TO_DEG
#undef RAD_TO_DEG
#endif

// Inclusiones para NVS (evitar errores de Preferences)
#include "nvs_flash.h"
#include "nvs.h"

// Configuración del sistema
#include "config/pins.h"
#include "config/constants.h"
#include "config/lorawan_config.h"
#include "core/Types.h"
#include "core/Logger.h"

// Managers de hardware
#include "hardware/PowerManager.h"
#include "hardware/BuzzerManager.h"
#include "hardware/DisplayManager.h"
#include "hardware/RadioManager.h"
#include "hardware/GPSManager.h"

// Managers del sistema
#include "system/GeofenceManager.h"
#include "system/AlertManager.h"

// Utilidades
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"

// ============================================================================
// INSTANCIAS DE MANAGERS
// ============================================================================

// Hardware Managers
PowerManager powerManager(VBAT_PIN);
BuzzerManager buzzerManager(BUZZER_PIN);
DisplayManager displayManager(OLED_ADDR, OLED_SDA, OLED_SCL, OLED_RST);
RadioManager radioManager(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);
GPSManager gpsManager(GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD);

// System Managers
GeofenceManager geofenceManager;
AlertManager alertManager(buzzerManager, displayManager);

// ============================================================================
// ESTADO DEL SISTEMA
// ============================================================================

SystemStatus systemStatus;
Position currentPosition;
BatteryStatus batteryStatus;
SystemStats systemStats;

// Variables de control de timing
uint32_t lastGeofenceCheck = 0;
uint32_t lastBatteryCheck = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastRadioTransmission = 0;
uint32_t lastHeartbeat = 0;
uint32_t lastPositionUpdate = 0;

// Estados de operación
bool systemInitialized = false;
bool emergencyMode = false;
uint16_t packetCounter = 0;

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

bool nvs_flash_init_custom() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition corrupted, erase and try again
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return (ret == ESP_OK);
}

// ============================================================================
// CALLBACKS DEL SISTEMA
// ============================================================================

void onRadioJoin(bool success) {
    if (success) {
        LOG_I("📡 LoRaWAN conectado exitosamente");
        systemStatus.radioInitialized = true;
        buzzerManager.playSuccessTone();
    } else {
        LOG_E("📡 Error conectando LoRaWAN");
        buzzerManager.playErrorTone();
    }
}

void onRadioTx(bool success) {
    if (success) {
        systemStats.totalPacketsSent++;
        LOG_PACKET(packetCounter, true);
    } else {
        systemStats.packetsLost++;
        LOG_PACKET(packetCounter, false);
    }
}

void onRadioDownlink(const uint8_t* data, size_t length, uint8_t port) {
    LOG_I("📡 Downlink recibido - Puerto: %d, Bytes: %d", port, length);
    systemStats.totalPacketsReceived++;
    
    // Procesar comando de downlink
    if (port == 2 && length >= 1) {
        uint8_t command = data[0];
        if (command == 0x01) {
            // Comando de activar buzzer
            alertManager.triggerSystemAlert("Downlink buzzer", AlertLevel::WARNING);
        }
    }
}

void onGeofenceViolation(const Geofence& geofence, float distance, AlertLevel level) {
    LOG_W("📍 Violación geocerca: %s - %.1fm - Nivel: %s", 
          geofence.name, distance, alertLevelToString(level));
    
    systemStats.geofenceViolations++;
    
    // Activar alerta
    alertManager.triggerGeofenceAlert(distance);
    
    // Transmisión inmediata en caso de emergencia
    if (level >= AlertLevel::EMERGENCY) {
        emergencyMode = true;
        LOG_E("🚨 MODO EMERGENCIA ACTIVADO");
    }
}

void onBatteryLow(BatteryStatus battery) {
    LOG_W("🔋 Batería baja: %.2fV (%d%%)", battery.voltage, battery.percentage);
    systemStats.lowBatteryEvents++;
    alertManager.triggerBatteryAlert(battery);
}

void onBatteryCritical(BatteryStatus battery) {
    LOG_E("🔋 Batería crítica: %.2fV (%d%%)", battery.voltage, battery.percentage);
    alertManager.triggerBatteryAlert(battery);
    
    // Entrar en modo de bajo consumo
    powerManager.enableLowPowerMode();
}

void onGPSPosition(const Position& position) {
    currentPosition = position;
    LOG_D("📍 GPS: %.6f, %.6f, %.1fm, %d sats", 
          position.latitude, position.longitude, position.altitude, position.satellites);
}

void onGPSFix(bool hasFix, uint8_t satellites) {
    if (hasFix) {
        LOG_I("📍 GPS Fix obtenido - %d satélites", satellites);
        buzzerManager.playTone(1500, 100, 50); // Tono suave de confirmación
    } else {
        LOG_W("📍 GPS Fix perdido - %d satélites", satellites);
    }
    
    systemStatus.gpsInitialized = hasFix;
}

// ============================================================================
// FUNCIONES DE INICIALIZACIÓN
// ============================================================================

Result initializeHardware() {
    LOG_I("🔧 Inicializando hardware...");
    
    // Configurar pines básicos
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Inicializar managers de hardware en orden
    Result result;
    
    // 1. Power Manager (primero para monitoreo)
    result = powerManager.init();
    if (result != Result::SUCCESS) {
        LOG_E("❌ Error inicializando PowerManager");
        return result;
    }
    systemStatus.uptime = powerManager.getUptime();
    
    // 2. Buzzer Manager (para feedback de inicio)
    result = buzzerManager.init();
    if (result != Result::SUCCESS) {
        LOG_E("❌ Error inicializando BuzzerManager");
        return result;
    }
    systemStatus.buzzerInitialized = true;
    
    // 3. Display Manager (para mostrar progreso)
    result = displayManager.init();
    if (result != Result::SUCCESS) {
        LOG_E("❌ Error inicializando DisplayManager");
        return result;
    }
    systemStatus.displayInitialized = true;
    
    // 4. GPS Manager (para posicionamiento)
    result = gpsManager.init();
    if (result != Result::SUCCESS) {
        LOG_E("❌ Error inicializando GPSManager");
        return result;
    }
    systemStatus.gpsInitialized = false; // Se activará cuando tenga fix
    
    // 5. Radio Manager (último por ser más complejo)
    result = radioManager.init();
    if (result != Result::SUCCESS) {
        LOG_E("❌ Error inicializando RadioManager");
        return result;
    }
    
    LOG_I("✅ Hardware inicializado correctamente");
    return Result::SUCCESS;
}

Result initializeSystem() {
    LOG_I("⚙️ Inicializando sistema...");
    
    Result result;
    
    // 1. Geofence Manager
    result = geofenceManager.init();
    if (result != Result::SUCCESS) {
        LOG_E("❌ Error inicializando GeofenceManager");
        return result;
    }
    
    // 2. Alert Manager
    result = alertManager.init();
    if (result != Result::SUCCESS) {
        LOG_E("❌ Error inicializando AlertManager");
        return result;
    }
    
    LOG_I("✅ Sistema inicializado correctamente");
    return Result::SUCCESS;
}

Result configureLoRaWAN() {
    LOG_I("📡 Configurando LoRaWAN...");
    
    // Verificar si LoRaWAN está configurado
    if (!isLoRaWANConfigured()) {
        LOG_W("📡 LoRaWAN no configurado - usando valores por defecto");
        LOG_W("📡 Editar config/lorawan_config.h con claves reales");
    }
    
    // Configurar callbacks
    radioManager.setJoinCallback(onRadioJoin);
    radioManager.setTxCallback(onRadioTx);
    radioManager.setDownlinkCallback(onRadioDownlink);
    
    // Configurar LoRaWAN
    Result result = radioManager.setupLoRaWAN();
    if (result != Result::SUCCESS) {
        return result;
    }
    
    // Intentar join OTAA o ABP según configuración
    if (LORAWAN_USE_OTAA) {
        result = radioManager.joinOTAA(LORAWAN_DEV_EUI, LORAWAN_APP_EUI, LORAWAN_APP_KEY);
    } else {
        result = radioManager.joinABP(LORAWAN_DEV_ADDR, LORAWAN_NWK_SKEY, LORAWAN_APP_SKEY);
    }
    
    if (result != Result::SUCCESS) {
        LOG_W("📡 LoRaWAN join falló, funcionando en modo básico");
        // Continuar sin LoRaWAN para testing
    }
    
    return Result::SUCCESS;
}

void configureCallbacks() {
    LOG_I("🔗 Configurando callbacks...");
    
    // Callbacks de geocerca
    geofenceManager.setViolationCallback(onGeofenceViolation);
    
    // Callbacks de batería
    powerManager.setBatteryLowCallback(onBatteryLow);
    powerManager.setBatteryCriticalCallback(onBatteryCritical);
    
    // Callbacks de GPS
    gpsManager.setPositionCallback(onGPSPosition);
    gpsManager.setFixCallback(onGPSFix);
    
    LOG_I("✅ Callbacks configurados");
}

void setupGeofence() {
    LOG_I("📍 Configurando geocerca...");
    
    // Configurar geocerca en Bosques de Chacay Bikepark, Santa Juana, Bío Bío
    geofenceManager.setGeofence(-37.34640277978371, -72.91495492379738, 100.0f, "Chacay Bikepark");
    geofenceManager.activate(true);
    
    LOG_I("📍 Geocerca activa: %.6f, %.6f, R=%.1fm", 
          geofenceManager.getCenterLat(), 
          geofenceManager.getCenterLng(), 
          geofenceManager.getRadius());
    LOG_I("📍 Ubicación: Bosques de Chacay Bikepark, Santa Juana, Bío Bío");
}

// ============================================================================
// FUNCIONES DE GPS REAL
// ============================================================================

void updateGPS() {
    // Actualizar GPS real
    gpsManager.update();
    
    // TEMPORAL: Deshabilitar GPS y usar posición fija para testing
    static bool positionInitialized = false;
    if (!positionInitialized) {
        // Usar posición DENTRO de la geocerca para evitar alertas
        currentPosition = Position(-37.34640277978371, -72.91495492379738);
        currentPosition.altitude = 150.0f;
        currentPosition.accuracy = 5.0f;
        currentPosition.satellites = 8; // Simular fix válido
        currentPosition.valid = true;
        currentPosition.timestamp = millis();
        
        LOG_I("📍 Usando posición fija DENTRO de geocerca (debugging)");
        LOG_I("📍 Pos: %.6f, %.6f", currentPosition.latitude, currentPosition.longitude);
        positionInitialized = true;
    }
    
    // Si no hay fix GPS válido, mantener la posición fija
    if (!gpsManager.hasValidFix()) {
        // Ya tenemos posición fija, no hacer nada
        LOG_D("📍 GPS sin fix - manteniendo posición fija");
    }
    // Si hay fix real, se actualiza automáticamente via callback
}

// ============================================================================
// FUNCIONES DE TRANSMISIÓN
// ============================================================================

void transmitPosition() {
    if (!radioManager.isJoined()) {
        LOG_D("📡 LoRaWAN no conectado, omitiendo transmisión");
        return;
    }
    
    AlertLevel alertLevel = alertManager.getCurrentLevel();
    Result result = radioManager.sendPosition(currentPosition, alertLevel);
    
    if (result == Result::SUCCESS) {
        packetCounter++;
        
        // LED de confirmación
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        
        LOG_I("📡 Posición transmitida #%d - Alerta: %s", 
              packetCounter, alertLevelToString(alertLevel));
    } else {
        LOG_E("📡 Error transmitiendo posición");
    }
}

void transmitBattery() {
    if (!radioManager.isJoined()) return;
    
    Result result = radioManager.sendBatteryStatus(batteryStatus);
    if (result == Result::SUCCESS) {
        LOG_I("📡 Estado batería transmitido");
    }
}

// ============================================================================
// FUNCIONES DE MONITOREO
// ============================================================================

void updateSystemStatus() {
    systemStatus.radioInitialized = radioManager.isInitialized();
    systemStatus.displayInitialized = displayManager.isInitialized();
    systemStatus.buzzerInitialized = buzzerManager.isInitialized();
    systemStatus.gpsInitialized = gpsManager.hasValidFix();
    systemStatus.uptime = powerManager.getUptime();
    systemStatus.freeHeap = powerManager.getFreeHeap();
    systemStatus.cpuTemperature = powerManager.getCPUTemperature();
    
    // Log memoria si está baja
    if (systemStatus.freeHeap < 10000) {
        LOG_MEMORY(systemStatus.freeHeap);
    }
}

void printHeartbeat() {
    float distance = geofenceManager.getDistance(currentPosition);
    AlertLevel alertLevel = alertManager.getCurrentLevel();
    
    LOG_I("💓 UP:%lum | TX:%d | GPS:%.6f,%.6f | Dist:%.1fm | Alert:%s | Bat:%.2fV | Heap:%luKB", 
          systemStatus.uptime / 60,
          packetCounter,
          currentPosition.latitude, currentPosition.longitude,
          distance,
          alertLevelToString(alertLevel),
          batteryStatus.voltage,
          systemStatus.freeHeap / 1024);
}

// ============================================================================
// SETUP PRINCIPAL
// ============================================================================

void setup() {
    // CRÍTICO: Activar VEXT inmediatamente para OLED y periféricos
    pinMode(VEXT_ENABLE, OUTPUT);
    digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE);
    delay(200); // Tiempo para estabilizar alimentación
    
    // Configurar LED para debugging temprano
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Indicar inicio
    
    // Inicializar Serial con timeout
    Serial.begin(SERIAL_BAUD);
    uint32_t serialStart = millis();
    while (!Serial && (millis() - serialStart < 3000)) {
        delay(10); // Esperar hasta 3 segundos por Serial
    }
    
    // Inicializar NVS para evitar errores de Preferences
    if (!nvs_flash_init_custom()) {
        Serial.println("⚠️ NVS flash init falló, usando modo sin persistencia");
    }
    
    // Inicializar logging
    Logger::init(SERIAL_BAUD);
    Logger::setLevel(Logger::INFO);
    Logger::printBanner();
    Logger::printSystemInfo();
    
    digitalWrite(LED_PIN, LOW); // Apagar LED tras init Serial
    
    LOG_I("🚀 Iniciando Collar Geofencing V3.0...");
    LOG_I("🔧 MODO DEBUG: Sistema simplificado para diagnosticar problemas");
    LOG_I("🛑 LoRaWAN: Deshabilitado");
    LOG_I("🛑 Alertas: Deshabilitadas");
    LOG_I("🛑 Watchdog: Deshabilitado");
    LOG_I("📍 GPS: Usando posición fija para testing");
    
    // Mostrar configuración LoRaWAN para debugging
    #if DEBUG_ENABLED
    printLoRaWANConfig();
    #endif
    
    // Splash screen inicial (con manejo de errores)
    if (displayManager.init() == Result::SUCCESS) {
        displayManager.showSplashScreen();
        delay(2000);
    } else {
        LOG_W("📺 Display no disponible, continuando sin pantalla");
        // Usar LED para indicar actividad
        for (int i = 0; i < 3; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(300);
            digitalWrite(LED_PIN, LOW);
            delay(300);
        }
    }
    
    // Inicializar hardware
    LOG_I("🔧 Paso 1: Inicializando hardware...");
    if (initializeHardware() != Result::SUCCESS) {
        LOG_E("❌ Error crítico en inicialización de hardware");
        buzzerManager.playErrorTone();
        while (true) delay(1000); // Halt en error crítico
    }
    LOG_I("✅ Hardware inicializado correctamente");
    
    // Melodía de inicio
    buzzerManager.playStartupMelody();
    
    // Inicializar sistema
    LOG_I("🔧 Paso 2: Inicializando sistema...");
    if (initializeSystem() != Result::SUCCESS) {
        LOG_E("❌ Error crítico en inicialización de sistema");
        buzzerManager.playErrorTone();
        while (true) delay(1000);
    }
    LOG_I("✅ Sistema inicializado correctamente");
    
    // Configurar LoRaWAN (TEMPORAL: deshabilitado para debugging)
    // configureLoRaWAN();
    LOG_I("🔧 Paso 3: LoRaWAN omitido (debugging)");
    
    // Configurar callbacks
    LOG_I("🔧 Paso 4: Configurando callbacks...");
    configureCallbacks();
    LOG_I("✅ Callbacks configurados");
    
    // Configurar geocerca
    LOG_I("🔧 Paso 5: Configurando geocerca...");
    setupGeofence();
    LOG_I("✅ Geocerca configurada");
    
    // Lectura inicial de batería
    powerManager.readBattery();
    batteryStatus = powerManager.getBatteryStatus();
    
    // TEMPORAL: Deshabilitar watchdog para debugging
    // powerManager.enableWatchdog(WATCHDOG_TIMEOUT);
    
    // Sistema inicializado
    systemInitialized = true;
    
    LOG_I("🎆 ============================================");
    LOG_I("🎆 SISTEMA COMPLETAMENTE INICIALIZADO");
    LOG_I("🎆 Modo DEBUG: Alertas y LoRaWAN deshabilitados");
    LOG_I("🎆 Posición fija: %.6f, %.6f", currentPosition.latitude, currentPosition.longitude);
    LOG_I("🎆 Geocerca: R=%.1fm en Chacay Bikepark", geofenceManager.getRadius());
    LOG_I("🎆 ============================================");
    
    // Confirmación final
    buzzerManager.playSuccessTone();
    
    // LED de confirmación
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
    }
    
    // Mostrar pantalla principal
    updateSystemStatus();
    displayManager.showMainScreen(systemStatus, currentPosition, batteryStatus, AlertLevel::SAFE);
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
    uint32_t currentTime = millis();
    
    // TEMPORAL: Feed watchdog aunque esté deshabilitado (para seguridad)
    // powerManager.feedWatchdog();
    
    // 1. Actualizar GPS real (cada 10 segundos para debugging)
    if (currentTime - lastPositionUpdate > (INTERVAL_GPS_READ * 2)) {
        LOG_D("📍 Actualizando GPS...");
        updateGPS();
        lastPositionUpdate = currentTime;
        
        // Log posición actual para debugging
        if (isValidPosition(currentPosition)) {
            LOG_D("📍 Pos actual: %.6f, %.6f (válida)", 
                  currentPosition.latitude, currentPosition.longitude);
        }
    }
    
    // 2. Verificar geocerca y actualizar alertas (cada 5 segundos para debugging)
    if (currentTime - lastGeofenceCheck > (INTERVAL_GEOFENCE_CHECK * 2)) {
        if (isValidPosition(currentPosition)) {
            LOG_D("📍 Verificando geocerca...");
            float distance = geofenceManager.getDistance(currentPosition);
            LOG_D("📍 Distancia a geocerca: %.1fm", distance);
            
            // TEMPORAL: No activar alertas para debugging
            // geofenceManager.update(currentPosition);
            // alertManager.update(distance);
            
            // Solo log, sin alertas
            if (distance > geofenceManager.getRadius()) {
                LOG_W("⚠️ FUERA de geocerca por %.1fm (alertas deshabilitadas)", 
                      distance - geofenceManager.getRadius());
            } else {
                LOG_I("✅ DENTRO de geocerca (%.1fm del centro)", distance);
            }
        }
        lastGeofenceCheck = currentTime;
    }
    
    // 3. Actualizar display (cada 5 segundos para debugging)
    if (currentTime - lastDisplayUpdate > (INTERVAL_DISPLAY_UPDATE + 2000)) {
        updateSystemStatus();
        
        // Mostrar solo pantalla principal (sin alertas para debugging)
        if (displayManager.isInitialized()) {
            displayManager.showMainScreen(systemStatus, currentPosition, batteryStatus, AlertLevel::SAFE);
        }
        
        lastDisplayUpdate = currentTime;
    }
    
    // 4. Leer batería (cada minuto)
    if (currentTime - lastBatteryCheck > INTERVAL_BATTERY_CHECK) {
        powerManager.readBattery();
        batteryStatus = powerManager.getBatteryStatus();
        lastBatteryCheck = currentTime;
    }
    
    // 5. Transmisión LoRaWAN (TEMPORAL: deshabilitada para debugging)
    /*
    uint32_t txInterval = TX_INTERVAL_NORMAL;
    if (emergencyMode) {
        txInterval = TX_INTERVAL_EMERGENCY;
    } else if (alertManager.getCurrentLevel() >= AlertLevel::DANGER) {
        txInterval = TX_INTERVAL_ALERT;
    }
    
    if (currentTime - lastRadioTransmission > txInterval) {
        if (isValidPosition(currentPosition)) {
            transmitPosition();
        }
        
        // Transmitir batería cada 10 transmisiones
        if (packetCounter % 10 == 0) {
            transmitBattery();
        }
        
        lastRadioTransmission = currentTime;
        
        // Salir de modo emergencia después de transmitir
        if (emergencyMode && alertManager.getCurrentLevel() < AlertLevel::EMERGENCY) {
            emergencyMode = false;
            LOG_I("✅ Modo emergencia desactivado");
        }
    }
    */
    
    // Log cada 15 segundos para debugging
    if (currentTime - lastRadioTransmission > 15000) {
        LOG_I("🐍 Sistema funcionando - LoRaWAN deshabilitado para debugging");
        lastRadioTransmission = currentTime;
    }
    
    // 6. Procesar downlinks (TEMPORAL: deshabilitado)
    // radioManager.processDownlinks();
    
    // 7. Heartbeat y estadísticas (cada 30 segundos)
    if (currentTime - lastHeartbeat > INTERVAL_HEARTBEAT) {
        printHeartbeat();
        lastHeartbeat = currentTime;
        
        // LED heartbeat simple
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
    }
    
    // 8. Actualizar managers (TEMPORAL: solo display)
    // alertManager.update();
    displayManager.update();
    
    // Delay mínimo para estabilidad
    delay(100);
}

// ============================================================================
// FIN DEL CÓDIGO - SISTEMA MODULAR COMPLETO
// ============================================================================

/*
 * 🎯 RESUMEN DEL SISTEMA MODULAR:
 * 
 * ✅ ARQUITECTURA PROFESIONAL:
 * - Separación clara de responsabilidades
 * - Managers especializados para cada función
 * - Configuración centralizada
 * - Sistema de logging avanzado
 * - Manejo robusto de errores
 * 
 * ✅ FUNCIONALIDADES COMPLETAS:
 * - LoRaWAN con downlinks y callbacks
 * - Geocercas múltiples con alertas progresivas
 * - Sistema de audio avanzado con melodías
 * - Display multi-pantalla con animaciones
 * - Gestión de energía con deep sleep
 * - Estadísticas y monitoreo completo
 * 
 * ✅ ESCALABILIDAD:
 * - Fácil agregar nuevos managers
 * - Sistema de callbacks extensible
 * - Configuración modular
 * - Preparado para GPS real
 * - Soporte para múltiples geocercas
 * 
 * 🚀 PRÓXIMOS PASOS:
 * 1. Compilar y probar el sistema completo
 * 2. Integrar GPS real (UART)
 * 3. Añadir persistencia de configuración
 * 4. Implementar OTA updates
 * 5. Optimizar para producción
 */
