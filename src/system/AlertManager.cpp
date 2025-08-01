#include "AlertManager.h"

// ============================================================================
// CONSTRUCTOR E INICIALIZACIÓN
// ============================================================================

AlertManager::AlertManager(BuzzerManager& buzzer, DisplayManager& display) :
    buzzerManager(buzzer),
    displayManager(display),
    initialized(false),
    enabled(true),
    currentLevel(AlertLevel::SAFE),
    previousLevel(AlertLevel::SAFE),
    alertActive(false),
    currentDistance(0.0f),
    alertStartTime(0),
    lastAlertTime(0),
    totalAlertsTriggered(0),
    maxLevelReached(AlertLevel::SAFE),
    escalationEnabled(true),
    autoStopEnabled(true),
    displayAlertsEnabled(true),
    audioAlertsEnabled(true),
    levelStartTime(0),
    escalationPending(false),
    currentAlertType(ALERT_GEOFENCE),
    alertCallback(nullptr),
    escalationCallback(nullptr)
{
    strcpy(currentReason, "Sistema OK");
    initializeThresholds();
}

Result AlertManager::init() {
    if (initialized) {
        return Result::SUCCESS;
    }
    
    LOG_I("🚨 Inicializando Alert Manager...");
    
    // Verificar que los managers requeridos estén inicializados
    if (!buzzerManager.isInitialized()) {
        LOG_E("❌ BuzzerManager no inicializado");
        return Result::ERROR_INIT;
    }
    
    if (!displayManager.isInitialized()) {
        LOG_E("❌ DisplayManager no inicializado");
        return Result::ERROR_INIT;
    }
    
    initialized = true;
    LOG_INIT("Alert Manager", true);
    
    return Result::SUCCESS;
}

bool AlertManager::isInitialized() const {
    return initialized;
}

// ============================================================================
// CONTROL PRINCIPAL DE ALERTAS
// ============================================================================

void AlertManager::update(float distanceToGeofence) {
    if (!initialized || !enabled) return;
    
    AlertLevel newLevel = calculateGeofenceLevel(distanceToGeofence);
    setAlertLevel(newLevel, distanceToGeofence);
    currentAlertType = ALERT_GEOFENCE;
}

void AlertManager::setAlertLevel(AlertLevel level, float distance) {
    if (!initialized || !isValidLevel(level)) return;
    
    previousLevel = currentLevel;
    currentLevel = level;
    currentDistance = distance;
    
    // Actualizar estadísticas
    if (level > maxLevelReached) {
        maxLevelReached = level;
    }
    
    // Detectar inicio/fin de alerta
    bool wasAlerting = alertActive;
    alertActive = (level > AlertLevel::SAFE);
    
    if (alertActive && !wasAlerting) {
        // Inicio de nueva alerta
        alertStartTime = millis();
        lastAlertTime = alertStartTime;
        totalAlertsTriggered++;
        levelStartTime = alertStartTime;
        escalationPending = false;
        
        LOG_I("🚨 Alerta iniciada - Nivel: %s, Distancia: %.1fm", 
              alertLevelToString(level), distance);
    } else if (!alertActive && wasAlerting) {
        // Fin de alerta
        uint32_t duration = millis() - alertStartTime;
        LOG_I("✅ Alerta finalizada - Duración: %lu segundos", duration / 1000);
        
        // Detener alertas activas
        buzzerManager.stopContinuousAlert();
    }
    
    // Detectar cambio de nivel
    if (level != previousLevel) {
        onLevelChange(previousLevel, level);
        levelStartTime = millis();
        escalationPending = false;
    }
    
    // Ejecutar alerta
    if (alertActive) {
        executeAlert();
    }
}

AlertLevel AlertManager::getCurrentLevel() const {
    return currentLevel;
}

bool AlertManager::isAlerting() const {
    return alertActive;
}

// ============================================================================
// CONTROL MANUAL DE ALERTAS
// ============================================================================

void AlertManager::startAlert(AlertLevel level, float distance) {
    copyReason("Alerta manual");
    currentAlertType = ALERT_MANUAL;
    setAlertLevel(level, distance);
}

void AlertManager::stopAlert() {
    if (alertActive) {
        setAlertLevel(AlertLevel::SAFE, 0.0f);
        copyReason("Sistema OK");
    }
}

void AlertManager::stopAllAlerts() {
    stopAlert();
    buzzerManager.stopContinuousAlert();
    escalationPending = false;
}

// ============================================================================
// TIPOS ESPECÍFICOS DE ALERTAS
// ============================================================================

void AlertManager::triggerGeofenceAlert(float distance) {
    currentAlertType = ALERT_GEOFENCE;
    snprintf(currentReason, sizeof(currentReason), "Geocerca: %.1fm", distance);
    
    AlertLevel level = calculateGeofenceLevel(distance);
    setAlertLevel(level, distance);
}

void AlertManager::triggerBatteryAlert(const BatteryStatus& battery) {
    currentAlertType = ALERT_BATTERY;
    snprintf(currentReason, sizeof(currentReason), "Batería: %.2fV", battery.voltage);
    
    AlertLevel level = calculateBatteryLevel(battery);
    setAlertLevel(level, 0.0f);
}

void AlertManager::triggerSystemAlert(const char* message, AlertLevel level) {
    currentAlertType = ALERT_SYSTEM;
    copyReason(message);
    setAlertLevel(level, 0.0f);
}

void AlertManager::triggerEmergencyAlert(const char* reason) {
    currentAlertType = ALERT_EMERGENCY;
    copyReason(reason);
    setAlertLevel(AlertLevel::EMERGENCY, 0.0f);
    
    LOG_E("🚨 EMERGENCIA: %s", reason);
}

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

void AlertManager::setEnabled(bool enabled) {
    this->enabled = enabled;
    
    if (!enabled) {
        stopAllAlerts();
    }
    
    LOG_I("🚨 Sistema de alertas %s", enabled ? "habilitado" : "deshabilitado");
}

bool AlertManager::isEnabled() const {
    return enabled;
}

void AlertManager::setGeofenceThresholds(float caution, float warning, float danger, float emergency) {
    thresholds.geofenceCaution = caution;
    thresholds.geofenceWarning = warning;
    thresholds.geofenceDanger = danger;
    thresholds.geofenceEmergency = emergency;
    
    LOG_I("🚨 Umbrales geocerca actualizados: %.1f/%.1f/%.1f/%.1fm", 
          caution, warning, danger, emergency);
}

void AlertManager::setBatteryThresholds(float lowVoltage, float criticalVoltage) {
    thresholds.batteryLow = lowVoltage;
    thresholds.batteryCritical = criticalVoltage;
    
    LOG_I("🚨 Umbrales batería actualizados: %.2f/%.2fV", lowVoltage, criticalVoltage);
}

// ============================================================================
// CONFIGURACIÓN DE COMPORTAMIENTO
// ============================================================================

void AlertManager::setEscalationEnabled(bool enabled) {
    escalationEnabled = enabled;
    if (!enabled) {
        escalationPending = false;
    }
}

void AlertManager::setAutoStopEnabled(bool enabled) {
    autoStopEnabled = enabled;
}

void AlertManager::setDisplayAlertsEnabled(bool enabled) {
    displayAlertsEnabled = enabled;
}

void AlertManager::setAudioAlertsEnabled(bool enabled) {
    audioAlertsEnabled = enabled;
    if (!enabled) {
        buzzerManager.stopContinuousAlert();
    }
}

// ============================================================================
// ESTADÍSTICAS
// ============================================================================

uint32_t AlertManager::getTotalAlertsTriggered() const {
    return totalAlertsTriggered;
}

uint32_t AlertManager::getAlertDuration() const {
    if (!alertActive) return 0;
    return millis() - alertStartTime;
}

uint32_t AlertManager::getTimeSinceLastAlert() const {
    if (alertActive) return 0;
    return millis() - lastAlertTime;
}

AlertLevel AlertManager::getMaxLevelReached() const {
    return maxLevelReached;
}

// ============================================================================
// CALLBACKS Y CONFIGURACIÓN
// ============================================================================

void AlertManager::setAlertCallback(AlertCallback callback) {
    alertCallback = callback;
}

void AlertManager::setEscalationCallback(EscalationCallback callback) {
    escalationCallback = callback;
}

void AlertManager::setEscalationConfig(const EscalationConfig& config) {
    escalationConfig = config;
}

AlertManager::EscalationConfig AlertManager::getEscalationConfig() const {
    return escalationConfig;
}

// ============================================================================
// UPDATE LOOP
// ============================================================================

void AlertManager::update() {
    if (!initialized) return;
    
    // Actualizar escalada automática
    if (escalationEnabled && alertActive) {
        updateEscalation();
    }
    
    // Actualizar managers de hardware
    buzzerManager.update();
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

void AlertManager::initializeThresholds() {
    // Configurar umbrales por defecto basados en constantes
    thresholds.geofenceCaution = CAUTION_DISTANCE;
    thresholds.geofenceWarning = WARNING_DISTANCE;
    thresholds.geofenceDanger = DANGER_DISTANCE;
    thresholds.geofenceEmergency = EMERGENCY_DISTANCE;
    thresholds.batteryLow = BATTERY_LOW;
    thresholds.batteryCritical = BATTERY_CRITICAL;
}

AlertLevel AlertManager::calculateGeofenceLevel(float distance) const {
    // Distancia negativa significa dentro de la geocerca
    if (distance <= thresholds.geofenceEmergency) {
        return AlertLevel::EMERGENCY;
    } else if (distance <= thresholds.geofenceDanger) {
        return AlertLevel::DANGER;
    } else if (distance <= thresholds.geofenceWarning) {
        return AlertLevel::WARNING;
    } else if (distance <= thresholds.geofenceCaution) {
        return AlertLevel::CAUTION;
    } else {
        return AlertLevel::SAFE;
    }
}

AlertLevel AlertManager::calculateBatteryLevel(const BatteryStatus& battery) const {
    if (battery.voltage <= thresholds.batteryCritical) {
        return AlertLevel::EMERGENCY;
    } else if (battery.voltage <= thresholds.batteryLow) {
        return AlertLevel::WARNING;
    } else {
        return AlertLevel::SAFE;
    }
}

// ============================================================================
// GESTIÓN DE ESCALADA
// ============================================================================

void AlertManager::updateEscalation() {
    if (!escalationConfig.enabled || !alertActive) return;
    
    uint32_t timeInLevel = millis() - levelStartTime;
    
    if (shouldEscalate() && timeInLevel >= escalationConfig.timeToEscalate) {
        escalateAlert();
    }
}

void AlertManager::escalateAlert() {
    AlertLevel nextLevel = getNextLevel(currentLevel);
    
    if (nextLevel != currentLevel) {
        LOG_W("🚨 Escalando alerta: %s -> %s", 
              alertLevelToString(currentLevel), alertLevelToString(nextLevel));
        
        AlertLevel oldLevel = currentLevel;
        setAlertLevel(nextLevel, currentDistance);
        
        if (escalationCallback) {
            escalationCallback(oldLevel, nextLevel);
        }
    }
}

bool AlertManager::shouldEscalate() const {
    return currentLevel < AlertLevel::EMERGENCY && // No escalar más allá de emergency
           (millis() - levelStartTime) >= escalationConfig.timeToEscalate;
}

AlertLevel AlertManager::getNextLevel(AlertLevel current) const {
    switch (current) {
        case AlertLevel::SAFE:      return AlertLevel::CAUTION;
        case AlertLevel::CAUTION:   return AlertLevel::WARNING;
        case AlertLevel::WARNING:   return AlertLevel::DANGER;
        case AlertLevel::DANGER:    return AlertLevel::EMERGENCY;
        case AlertLevel::EMERGENCY: return AlertLevel::EMERGENCY; // Ya en máximo
        default:                    return current;
    }
}

// ============================================================================
// EJECUCIÓN DE ALERTAS
// ============================================================================

void AlertManager::executeAlert() {
    if (!alertActive) return;
    
    // Actualizar buzzer
    if (audioAlertsEnabled) {
        updateBuzzer();
    }
    
    // Actualizar display
    if (displayAlertsEnabled) {
        updateDisplay();
    }
    
    // Log periódico
    static uint32_t lastLog = 0;
    if (millis() - lastLog > 10000) { // Log cada 10 segundos
        logAlert();
        lastLog = millis();
    }
}

void AlertManager::updateDisplay() {
    // Solo actualizar si el display no está mostrando alerta
    if (displayManager.getCurrentScreenMode() != DisplayManager::SCREEN_ALERT) {
        displayManager.showAlertScreen(currentLevel, currentDistance);
    }
}

void AlertManager::updateBuzzer() {
    // Iniciar alerta continua en el buzzer
    if (!buzzerManager.isPlaying()) {
        buzzerManager.startContinuousAlert(currentLevel);
    }
}

void AlertManager::logAlert() {
    LOG_GEOFENCE(currentDistance, static_cast<uint8_t>(currentLevel));
}

// ============================================================================
// CALLBACKS Y EVENTOS
// ============================================================================

void AlertManager::triggerCallbacks() {
    if (alertCallback) {
        alertCallback(currentLevel, currentDistance, currentReason);
    }
}

void AlertManager::onLevelChange(AlertLevel oldLevel, AlertLevel newLevel) {
    LOG_I("🚨 Cambio nivel alerta: %s -> %s", 
          alertLevelToString(oldLevel), alertLevelToString(newLevel));
    
    triggerCallbacks();
    
    if (escalationCallback && newLevel > oldLevel) {
        escalationCallback(oldLevel, newLevel);
    }
}

// ============================================================================
// UTILIDADES
// ============================================================================

bool AlertManager::isValidLevel(AlertLevel level) const {
    return level >= AlertLevel::SAFE && level <= AlertLevel::EMERGENCY;
}

const char* AlertManager::getAlertTypeString(AlertType type) const {
    switch (type) {
        case ALERT_GEOFENCE:    return "Geocerca";
        case ALERT_BATTERY:     return "Batería";
        case ALERT_SYSTEM:      return "Sistema";
        case ALERT_EMERGENCY:   return "Emergencia";
        case ALERT_MANUAL:      return "Manual";
        default:                return "Desconocido";
    }
}

void AlertManager::copyReason(const char* reason) {
    if (reason) {
        strncpy(currentReason, reason, sizeof(currentReason) - 1);
        currentReason[sizeof(currentReason) - 1] = '\0';
    }
}
