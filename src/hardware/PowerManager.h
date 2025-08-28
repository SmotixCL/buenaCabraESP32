#pragma once
#include <Arduino.h>
#include "../config/pins.h"
#include "../config/constants.h"
#include "../core/Types.h"

// Definición de WATCHDOG_TIMEOUT si no existe
#ifndef WATCHDOG_TIMEOUT
#define WATCHDOG_TIMEOUT 30
#endif

// Constantes de batería
#ifndef BATTERY_LOW
#define BATTERY_LOW 3.3 // Voltaje bajo
#endif
#ifndef BATTERY_CRITICAL
#define BATTERY_CRITICAL 3.0 // Voltaje crítico
#endif

// Constantes ADC para batería
#define ADC_CTRL_PIN 37 // Pin de control ADC en Heltec V3
#ifndef VBAT_REFERENCE
#define VBAT_REFERENCE 3.3 // Voltaje de referencia ADC
#endif
#ifndef VBAT_DIVIDER
#define VBAT_DIVIDER 4.9 // Divisor de voltaje
#endif
#ifndef VBAT_RESOLUTION
#define VBAT_RESOLUTION 4095.0 // Resolución ADC (12 bits)
#endif

// Pines de expansión adicionales
#ifndef EXP_PIN_3
#define EXP_PIN_3 6
#endif
#ifndef EXP_PIN_4
#define EXP_PIN_4 15
#endif
/*
 * ============================================================================
 * POWER MANAGER - GESTIÓN DE ENERGÍA Y BATERÍA
 * ============================================================================
 */

class PowerManager
{
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
    uint32_t getUptime() const;      // Segundos desde encendido
    uint32_t getFreeHeap() const;    // Memoria libre
    float getCPUTemperature() const; // Temperatura estimada

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
    float calculateBatteryPercentage(float voltage);
    void updateBatteryStatus();
    void triggerBatteryCallbacks();

    // Configuración de pines para low power
    void configurePinsForSleep();
    void restorePinsAfterSleep();
};
