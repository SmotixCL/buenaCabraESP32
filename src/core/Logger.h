#pragma once
#include <Arduino.h> 
#include "../config/pins.h"
#include "../config/constants.h"  
/*
 * ============================================================================
 * SISTEMA DE LOGGING AVANZADO - COLLAR GEOFENCING
 * ============================================================================
 */

class Logger {
public:
    enum Level { ERROR = 1, WARN, INFO, DEBUG };
    static void init(uint32_t baudRate);
    static void setLevel(Level level);
    static Level getLevel();
    
    // Métodos de logging
    static void error(const char* message, ...);
    static void warn(const char* message, ...);
    static void info(const char* message, ...);
    static void debug(const char* message, ...);
    
    // Métodos específicos del sistema
    static void logSystemInit(const char* component, bool success);
    static void logGeofenceEvent(float distance, uint8_t alertLevel);
    static void logPacketSent(uint16_t sequenceNumber, bool success);
    static void logBatteryStatus(float voltage, uint8_t percentage);
    static void logGPSPosition(double lat, double lng, bool valid);
    static void logMemoryStatus(uint32_t freeHeap);
    
    // Banner y información del sistema
    static void printBanner();
    static void printSystemInfo();
    
private:
    static Level currentLevel;
    static bool initialized;
    static uint32_t bootTime;
    
    static void logWithLevel(Level level, const char* prefix, const char* message, va_list args);
    static const char* getLevelString(Level level);
    static const char* getLevelEmoji(Level level);
    static uint32_t getUptime();
};

// ============================================================================
// MACROS DE CONVENIENCIA
// ============================================================================
#define LOG_E(...) Logger::error(__VA_ARGS__)
#define LOG_W(...) Logger::warn(__VA_ARGS__)
#define LOG_I(...) Logger::info(__VA_ARGS__)
#define LOG_D(...) Logger::debug(__VA_ARGS__)

#define LOG_INIT(component, success) Logger::logSystemInit(component, success)
#define LOG_GEOFENCE(distance, level) Logger::logGeofenceEvent(distance, level)
#define LOG_PACKET(seq, success) Logger::logPacketSent(seq, success)
#define LOG_BATTERY(voltage, percentage) Logger::logBatteryStatus(voltage, percentage)
#define LOG_GPS(lat, lng, valid) Logger::logGPSPosition(lat, lng, valid)

// Niveles de logging
#define LOG_LEVEL 5
#define LOG_LEVEL_DEBUG 4