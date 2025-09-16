#include "AlertManager.h"
#include "../core/Logger.h"

// ============================================================================
// CONSTRUCTOR E INICIALIZACIÓN
// ============================================================================

AlertManager::AlertManager(BuzzerManager &buzzer, DisplayManager &display) : buzzerManager(buzzer),
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
                                                                             displayAlertsEnabled(false), // 🔥 DESHABILITADO - No mostrar pantallas emergentes
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

Result AlertManager::init()
{
    if (initialized)
    {
        return Result::SUCCESS;
    }

    LOG_I("🚨 Inicializando Alert Manager...");

    // Verificar que los managers requeridos estén inicializados
    if (!buzzerManager.isInitialized())
    {
        LOG_E("❌ BuzzerManager no inicializado");
        return Result::ERROR_INIT;
    }

    if (!displayManager.isInitialized())
    {
        LOG_E("❌ DisplayManager no inicializado");
        return Result::ERROR_INIT;
    }

    initialized = true;
    LOG_INIT("Alert Manager", true);

    return Result::SUCCESS;
}

bool AlertManager::isInitialized() const
{
    return initialized;
}

// ============================================================================
// CONTROL PRINCIPAL DE ALERTAS
// ============================================================================

void AlertManager::update(float distanceToGeofence)
{
    if (!initialized || !enabled)
        return;

    AlertLevel newLevel = calculateGeofenceLevel(distanceToGeofence);
    setAlertLevel(newLevel, distanceToGeofence);
    currentAlertType = ALERT_GEOFENCE;
}

void AlertManager::setAlertLevel(AlertLevel level, float distance)
{
    if (!initialized || !isValidLevel(level))
        return;

    previousLevel = currentLevel;
    currentLevel = level;
    currentDistance = distance;

    // Actualizar estadísticas
    if (level > maxLevelReached)
    {
        maxLevelReached = level;
    }

    // Detectar inicio/fin de alerta
    bool wasAlerting = alertActive;           // wasAlerting: Estado previo
    alertActive = (level > AlertLevel::SAFE); // alertActive: Estado actual

    // Dar inicio si ahora hay que alertar y previamente no estábamos alertando
    if (alertActive && !wasAlerting)
    {
        // Inicio de nueva alerta
        alertStartTime = millis();
        lastAlertTime = alertStartTime;
        totalAlertsTriggered++;
        levelStartTime = alertStartTime;
        escalationPending = false;

        LOG_I("🚨 Alerta iniciada - Nivel: %s, Distancia: %.1fm",
              alertLevelToString(level), distance);

        buzzerManager.startContinuousAlert(level);
    }
    // Finalizar si ahora no hay que alertar y previamente estábamos alertando
    else if (!alertActive && wasAlerting)
    {
        // Fin de alerta
        uint32_t duration = millis() - alertStartTime;
        LOG_I("✅ Alerta finalizada - Duración: %lu segundos", duration / 1000);

        // Detener alertas activas
        buzzerManager.stopContinuousAlert();
    }

    // Detectar cambio de nivel
    if (level != previousLevel)
    {
        onLevelChange(previousLevel, level);
        levelStartTime = millis();
        escalationPending = false;
    }

    // Ejecutar alerta
    if (alertActive)
    {
        executeAlert();
    }
}

AlertLevel AlertManager::getCurrentLevel() const
{
    return currentLevel;
}

bool AlertManager::isAlerting() const
{
    return alertActive;
}

// ============================================================================
// CONTROL MANUAL DE ALERTAS
// ============================================================================

void AlertManager::startAlert(AlertLevel level, float distance)
{
    copyReason("Alerta manual");
    currentAlertType = ALERT_MANUAL;
    setAlertLevel(level, distance);
}

void AlertManager::stopAlert()
{
    if (alertActive)
    {
        setAlertLevel(AlertLevel::SAFE, 0.0f);
        copyReason("Sistema OK");
    }
}

void AlertManager::stopAllAlerts()
{
    stopAlert();
    buzzerManager.stopContinuousAlert();
    escalationPending = false;
}

// ============================================================================
// TIPOS ESPECÍFICOS DE ALERTAS
// ============================================================================

void AlertManager::triggerGeofenceAlert(float distance)
{
    currentAlertType = ALERT_GEOFENCE;
    snprintf(currentReason, sizeof(currentReason), "Geocerca: %.1fm", distance);

    AlertLevel level = calculateGeofenceLevel(distance);
    setAlertLevel(level, distance);
}

void AlertManager::triggerBatteryAlert(const BatteryStatus &battery)
{
    currentAlertType = ALERT_BATTERY;
    snprintf(currentReason, sizeof(currentReason), "Batería: %.2fV", battery.voltage);

    AlertLevel level = calculateBatteryLevel(battery);
    setAlertLevel(level, 0.0f);
}

void AlertManager::triggerSystemAlert(const char *message, AlertLevel level)
{
    currentAlertType = ALERT_SYSTEM;
    copyReason(message);
    setAlertLevel(level, 0.0f);
}

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

void AlertManager::setEnabled(bool enabled)
{
    this->enabled = enabled;

    if (!enabled)
    {
        stopAllAlerts();
    }

    LOG_I("🚨 Sistema de alertas %s", enabled ? "habilitado" : "deshabilitado");
}

bool AlertManager::isEnabled() const
{
    return enabled;
}

void AlertManager::setBatteryThresholds(float lowVoltage, float criticalVoltage)
{
    thresholds.batteryLow = lowVoltage;
    thresholds.batteryCritical = criticalVoltage;

    LOG_I("🚨 Umbrales batería actualizados: %.2f/%.2fV", lowVoltage, criticalVoltage);
}

// ============================================================================
// CONFIGURACIÓN DE COMPORTAMIENTO
// ============================================================================

void AlertManager::setEscalationEnabled(bool enabled)
{
    escalationEnabled = enabled;
    if (!enabled)
    {
        escalationPending = false;
    }
}

void AlertManager::setAutoStopEnabled(bool enabled)
{
    autoStopEnabled = enabled;
}

void AlertManager::setDisplayAlertsEnabled(bool enabled)
{
    displayAlertsEnabled = enabled;
}

void AlertManager::setAudioAlertsEnabled(bool enabled)
{
    audioAlertsEnabled = enabled;
    if (!enabled)
    {
        buzzerManager.stopContinuousAlert();
    }
}

// ============================================================================
// ESTADÍSTICAS
// ============================================================================

uint32_t AlertManager::getTotalAlertsTriggered() const
{
    return totalAlertsTriggered;
}

uint32_t AlertManager::getAlertDuration() const
{
    if (!alertActive)
        return 0;
    return millis() - alertStartTime;
}

uint32_t AlertManager::getTimeSinceLastAlert() const
{
    if (alertActive)
        return 0;
    return millis() - lastAlertTime;
}

AlertLevel AlertManager::getMaxLevelReached() const
{
    return maxLevelReached;
}

// ============================================================================
// CALLBACKS Y CONFIGURACIÓN
// ============================================================================

void AlertManager::setAlertCallback(AlertCallback callback)
{
    alertCallback = callback;
}

void AlertManager::setEscalationCallback(EscalationCallback callback)
{
    escalationCallback = callback;
}

void AlertManager::setEscalationConfig(const EscalationConfig &config)
{
    escalationConfig = config;
}

AlertManager::EscalationConfig AlertManager::getEscalationConfig() const
{
    return escalationConfig;
}

// ============================================================================
// UPDATE LOOP
// ============================================================================

void AlertManager::update()
{
    if (!initialized)
        return;

    // Actualizar escalada automática
    if (escalationEnabled && alertActive)
    {
        updateEscalation();
    }

    // Actualizar managers de hardware
    buzzerManager.update();
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

void AlertManager::initializeThresholds()
{
    // Configurar umbrales por defecto basados en constantes
    thresholds.geofenceCaution = CAUTION_DISTANCE;
    thresholds.geofenceWarning = WARNING_DISTANCE;
    thresholds.batteryLow = BATTERY_LOW;
    thresholds.batteryCritical = BATTERY_CRITICAL;
}

AlertLevel AlertManager::calculateGeofenceLevel(float distance) const
{
    // Distancia negativa significa dentro de la geocerca. Entra en el último caso.

    // Distancia positiva significa fuera de la geocerca - A MAYOR distancia, MAYOR alerta
    if (distance >= thresholds.geofenceWarning)
    {
        return AlertLevel::WARNING;
    }
    else if (distance >= thresholds.geofenceCaution)
    {
        return AlertLevel::CAUTION;
    }
    else
    {
        return AlertLevel::SAFE; // Dentro o muy cerca de la geocerca
    }
}

AlertLevel AlertManager::calculateBatteryLevel(const BatteryStatus &battery) const
{
    if (battery.voltage <= thresholds.batteryCritical)
    {
        return AlertLevel::WARNING;
    }
    else if (battery.voltage <= thresholds.batteryLow)
    {
        return AlertLevel::CAUTION;
    }
    else
    {
        return AlertLevel::SAFE;
    }
}

// ============================================================================
// GESTIÓN DE ESCALADA
// ============================================================================

void AlertManager::updateEscalation()
{
    if (!escalationConfig.enabled || !alertActive)
    {
        LOG_D("No se actualizó la escalación de alerta");
        return;
    }

    uint32_t timeInLevel = millis() - levelStartTime;

    if (shouldEscalate() && timeInLevel >= escalationConfig.timeToEscalate)
    {
        LOG_D("Se actualizó la escalación de alerta");
        escalateAlert();
    }
}

void AlertManager::escalateAlert()
{
    AlertLevel nextLevel = getNextLevel(currentLevel);

    if (nextLevel != currentLevel)
    {
        LOG_W("🚨 Escalando alerta: %s -> %s",
              alertLevelToString(currentLevel), alertLevelToString(nextLevel));

        AlertLevel oldLevel = currentLevel;
        setAlertLevel(nextLevel, currentDistance);

        if (escalationCallback)
        {
            escalationCallback(oldLevel, nextLevel);
        }
    }
}

bool AlertManager::shouldEscalate() const
{
    return currentLevel < AlertLevel::WARNING && // No escalar más allá de warning
           (millis() - levelStartTime) >= escalationConfig.timeToEscalate;
}

AlertLevel AlertManager::getNextLevel(AlertLevel current) const
{
    switch (current)
    {
    case AlertLevel::SAFE:
        return AlertLevel::CAUTION;
    case AlertLevel::CAUTION:
        return AlertLevel::WARNING;
    case AlertLevel::WARNING:
        return AlertLevel::WARNING; // MÁXIMO
    default:
        return current;
    }
}

// ============================================================================
// EJECUCIÓN DE ALERTAS
// ============================================================================

void AlertManager::executeAlert()
{
    if (!alertActive)
        return;

    // Actualizar buzzer solamente - sin interrumpir pantallas
    if (audioAlertsEnabled)
    {
        LOG_D("Ejecutamos updateBuzzer() dentro de executeAlert");
        updateBuzzer();
    }

    // 🔥 ELIMINADO: updateDisplay() - Ya no interrumpe las pantallas predefinidas

    // Log de que ejecutamos la Alerta
    LOG_GEOFENCE(currentDistance, static_cast<uint8_t>(currentLevel));
}

void AlertManager::updateDisplay()
{
    // 🔥 MÉTODO DESHABILITADO - No mostrar pantallas emergentes de alerta
    // La información de alerta ahora se muestra solo en la pantalla principal
    // como parte del estado normal del sistema, sin interrumpir la navegación

    // Las alertas se pueden ver en:
    // 1. Pantalla principal: Muestra el nivel de alerta actual
    // 2. Pantalla de geocerca: Muestra el estado y distancia
    // 3. Audio: Buzzer sigue funcionando para alertas sonoras
}

void AlertManager::updateBuzzer()
{
    // Ejecutar el update cuando la alerta continua está activa
    if (buzzerManager.isContinousAlertActive())
    {
        LOG_D("Ejecutamos updateContinuousAlert");
        buzzerManager.updateContinuousAlert();
    }
}

// ============================================================================
// CALLBACKS Y EVENTOS
// ============================================================================

void AlertManager::triggerCallbacks()
{
    if (alertCallback)
    {
        alertCallback(currentLevel, currentDistance, currentReason);
    }
}

void AlertManager::onLevelChange(AlertLevel oldLevel, AlertLevel newLevel)
{
    LOG_I("🚨 Cambio nivel alerta: %s -> %s",
          alertLevelToString(oldLevel), alertLevelToString(newLevel));

    triggerCallbacks();

    if (escalationCallback && newLevel > oldLevel)
    {
        escalationCallback(oldLevel, newLevel);
    }
}

// ============================================================================
// UTILIDADES
// ============================================================================

bool AlertManager::isValidLevel(AlertLevel level) const
{
    return level >= AlertLevel::SAFE && level <= AlertLevel::WARNING;
}

const char *AlertManager::getAlertTypeString(AlertType type) const
{
    switch (type)
    {
    case ALERT_GEOFENCE:
        return "Geocerca";
    case ALERT_BATTERY:
        return "Batería";
    case ALERT_SYSTEM:
        return "Sistema";
    case ALERT_MANUAL:
        return "Manual";
    default:
        return "Desconocido";
    }
}

void AlertManager::copyReason(const char *reason)
{
    if (reason)
    {
        strncpy(currentReason, reason, sizeof(currentReason) - 1);
        currentReason[sizeof(currentReason) - 1] = '\0';
    }
}
