#pragma once
#include <Arduino.h>
#include "config/pins.h"
#include "config/constants.h"
#include "core/Types.h"
#include "core/Logger.h"

/*
 * ============================================================================
 * POWER MANAGER - GESTIÓN DE ENERGÍA Y BATERÍA
 * ============================================================================
 */

class PowerManager {
public:
    PowerManager(uint8_t batteryPin = VBAT_PIN);
    
    // Inicialización y configuración
    Result init();
    bool isInitialized() const;
    
    // Lectura de batería
    void readBattery();
    BatteryStatus getBatteryStatus() const;
    float getVoltage() const;
    uint8_t getPercentage() const;
    bool isLow() const;
    bool isCritical() const;
    
    // Gestión de energía
    void enableLowPowerMode();
    void disableLowPowerMode();
    void prepareForDeepSleep(uint64_t sleepTimeUs = 0);
    void wakeFromDeepSleep();
    
    // Watchdog
    void enableWatchdog(uint32_t timeoutSeconds = WATCHDOG_TIMEOUT);
    void disableWatchdog();
    void feedWatchdog();
    
    // Información del sistema
    uint32_t getUptime() const;        // Segundos desde encendido
    uint32_t getFreeHeap() const;      // Memoria libre
    float getCPUTemperature() const;   // Temperatura estimada
    
    // Callbacks para eventos de batería
    typedef void (*BatteryCallback)(BatteryStatus status);
    void setBatteryLowCallback(BatteryCallback callback);
    void setBatteryCriticalCallback(BatteryCallback callback);
    
private:
    uint8_t vbatPin;
    BatteryStatus batteryStatus;
    bool initialized;
    bool lowPowerModeEnabled;
    uint32_t startTime;
    
    // Callbacks
    BatteryCallback lowBatteryCallback;
    BatteryCallback criticalBatteryCallback;
    
    // Variables para promedio de batería
    static const uint8_t BATTERY_SAMPLES = 10;
    float batterySamples[BATTERY_SAMPLES];
    uint8_t currentSample;
    bool samplesReady;
    
    // Métodos privados
    float readVoltageRaw();
    float calculateAverageVoltage();
    void updateBatteryStatus();
    void triggerBatteryCallbacks();
    
    // Configuración de pines para low power
    void configurePinsForSleep();
    void restorePinsAfterSleep();
};
