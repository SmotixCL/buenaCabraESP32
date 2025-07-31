/**
 * ============================================================================
 * COLLAR GEOFENCING - MÓDULO HARDWARE
 * ============================================================================
 * 
 * Gestión centralizada de todo el hardware del collar
 * 
 * @file Hardware.h
 * @version 3.0
 */

#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include "config.h"

class Hardware {
private:
    static HardwareStatus status;
    static bool initialized;

public:
    // *** INICIALIZACIÓN ***
    static bool init();
    static bool initPins();
    static bool initSPI();
    static bool initI2C();
    static bool initSerial();
    
    // *** CONTROL DE PINS ***
    static void ledOn();
    static void ledOff();
    static void ledBlink(uint8_t times, uint16_t duration = 200);
    static void ledToggle();
    
    // *** INFORMACIÓN DEL SISTEMA ***
    static HardwareStatus getStatus();
    static bool isInitialized();
    static void printSystemInfo();
    static void printPinMap();
    
    // *** GESTIÓN DE ENERGÍA ***
    static void enableLowPowerMode();
    static void disableLowPowerMode();
    static void prepareForSleep();
    static void wakeFromSleep();
    
    // *** UTILIDADES ***
    static uint32_t getUptime();
    static uint32_t getFreeHeap();
    static float getCPUTemperature();
    
    // *** WATCHDOG ***
    static void feedWatchdog();
    static void enableWatchdog();
    static void disableWatchdog();
};

#endif // HARDWARE_H