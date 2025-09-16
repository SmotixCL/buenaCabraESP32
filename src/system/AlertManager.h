#pragma once
#include <Arduino.h>
#include "../config/pins.h"
#include "../config/constants.h"
#include "../core/Types.h"
#include "hardware/BuzzerManager.h"
#include "hardware/DisplayManager.h"

/*
 * ============================================================================
 * ALERT MANAGER - GESTIÓN CENTRALIZADA DE ALERTAS
 * ============================================================================
 */

// ===========================================================================
// CONSTANTES PARA LAS DISTANCIAS A LAS QUE SE ACTIVA CADA ALERTA
// ===========================================================================
#define CAUTION_DISTANCE -15 // De -15 a 0 caution -> Alerta sonora 1 [Gradiente de 3 tonos, cada vez más agudo]
#define WARNING_DISTANCE 0   // De 0 a 30 alerta warning -> Alerta sonora 2 [Intermitente]

class AlertManager
{
public:
    AlertManager(BuzzerManager &buzzer, DisplayManager &display);

    // Inicialización
    Result init();
    bool isInitialized() const;

    // Control principal de alertas
    void update(float distanceToGeofence);
    void setAlertLevel(AlertLevel level, float distance = 0.0f);
    AlertLevel getCurrentLevel() const;
    bool isAlerting() const;

    // Control manual de alertas
    void startAlert(AlertLevel level, float distance = 0.0f);
    void stopAlert();
    void stopAllAlerts();

    // Tipos específicos de alertas
    void triggerGeofenceAlert(float distance);
    void triggerBatteryAlert(const BatteryStatus &battery);
    void triggerSystemAlert(const char *message, AlertLevel level = AlertLevel::WARNING);

    // Configuración de alertas
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setBatteryThresholds(float lowVoltage, float criticalVoltage);

    // Configuración de comportamiento
    void setEscalationEnabled(bool enabled);
    void setAutoStopEnabled(bool enabled);
    void setDisplayAlertsEnabled(bool enabled);
    void setAudioAlertsEnabled(bool enabled);

    // Estadísticas y estado
    uint32_t getTotalAlertsTriggered() const;
    uint32_t getAlertDuration() const;      // Duración de la alerta actual (ms)
    uint32_t getTimeSinceLastAlert() const; // Tiempo desde última alerta (ms)
    AlertLevel getMaxLevelReached() const;

    // Callbacks para eventos de alerta
    typedef void (*AlertCallback)(AlertLevel level, float distance, const char *reason);
    typedef void (*EscalationCallback)(AlertLevel oldLevel, AlertLevel newLevel);

    void setAlertCallback(AlertCallback callback);
    void setEscalationCallback(EscalationCallback callback);

    // Update loop (llamar desde loop principal)
    void update();

    // Configuración de escalada automática
    struct EscalationConfig
    {
        bool enabled;
        uint32_t timeToEscalate; // Tiempo en ms para escalar al siguiente nivel
        bool resetOnImprovement; // Si mejorar resetea el tiempo de escalada

        EscalationConfig() : enabled(true), timeToEscalate(30000), resetOnImprovement(true) {}
    };

    void setEscalationConfig(const EscalationConfig &config);
    EscalationConfig getEscalationConfig() const;

private:
    // Referencias a managers de hardware
    BuzzerManager &buzzerManager;
    DisplayManager &displayManager;

    bool initialized;
    bool enabled;

    // Estado actual de alertas
    AlertLevel currentLevel;
    AlertLevel previousLevel;
    bool alertActive;
    float currentDistance;
    char currentReason[64];

    // Timing y estadísticas
    uint32_t alertStartTime;
    uint32_t lastAlertTime;
    uint32_t totalAlertsTriggered;
    AlertLevel maxLevelReached;

    // Configuración de umbrales
    struct AlertThresholds
    {
        float geofenceCaution;
        float geofenceWarning;
        float batteryLow;
        float batteryCritical;
    } thresholds;

    // Configuración de comportamiento
    bool escalationEnabled;
    bool autoStopEnabled;
    bool displayAlertsEnabled;
    bool audioAlertsEnabled;

    // Sistema de escalada
    EscalationConfig escalationConfig;
    uint32_t levelStartTime;
    bool escalationPending;

    // Callbacks
    AlertCallback alertCallback;
    EscalationCallback escalationCallback;

    // Estado de alertas por tipo
    enum AlertType
    {
        ALERT_GEOFENCE,
        ALERT_BATTERY,
        ALERT_SYSTEM,
        ALERT_MANUAL
    };

    AlertType currentAlertType;

    // Métodos privados
    void initializeThresholds();
    AlertLevel calculateGeofenceLevel(float distance) const;
    AlertLevel calculateBatteryLevel(const BatteryStatus &battery) const;

    // Gestión de escalada
    void updateEscalation();
    void escalateAlert();
    bool shouldEscalate() const;
    AlertLevel getNextLevel(AlertLevel current) const;

    // Ejecución de alertas
    void executeAlert();
    void updateDisplay();
    void updateBuzzer();

    // Callbacks internos
    void triggerCallbacks();
    void onLevelChange(AlertLevel oldLevel, AlertLevel newLevel);

    // Validación
    bool isValidLevel(AlertLevel level) const;

    // Utilidades
    const char *getAlertTypeString(AlertType type) const;
    void copyReason(const char *reason);
};
