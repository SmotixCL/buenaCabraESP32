#include "Logger.h"
#include <stdarg.h>

// ============================================================================
// VARIABLES ESTÁTICAS
// ============================================================================
Logger::Level Logger::currentLevel = Logger::INFO;
bool Logger::initialized = false;
uint32_t Logger::bootTime = 0;

// ============================================================================
// IMPLEMENTACIÓN PÚBLICA
// ============================================================================

void Logger::init(uint32_t baudRate) {
    if (initialized) return;
    
    Serial.begin(baudRate);
    
    // Esperar conexión serial o timeout
    uint32_t start = millis();
    while (!Serial && (millis() - start) < SERIAL_TIMEOUT) {
        delay(10);
    }
    
    bootTime = millis();
    initialized = true;
    
    printBanner();
    info("Logger iniciado - Nivel: %s", getLevelString(currentLevel));
}

void Logger::setLevel(Level level) {
    currentLevel = level;
    info("Nivel de logging cambiado a: %s", getLevelString(level));
}

Logger::Level Logger::getLevel() {
    return currentLevel;
}

void Logger::error(const char* message, ...) {
    if (currentLevel >= ERROR) {
        va_list args;
        va_start(args, message);
        logWithLevel(ERROR, "ERROR", message, args);
        va_end(args);
    }
}

void Logger::warn(const char* message, ...) {
    if (currentLevel >= WARN) {
        va_list args;
        va_start(args, message);
        logWithLevel(WARN, "WARN", message, args);
        va_end(args);
    }
}

void Logger::info(const char* message, ...) {
    if (currentLevel >= INFO) {
        va_list args;
        va_start(args, message);
        logWithLevel(INFO, "INFO", message, args);
        va_end(args);
    }
}

void Logger::debug(const char* message, ...) {
    if (currentLevel >= DEBUG) {
        va_list args;
        va_start(args, message);
        logWithLevel(DEBUG, "DEBUG", message, args);
        va_end(args);
    }
}

// ============================================================================
// MÉTODOS ESPECÍFICOS DEL SISTEMA
// ============================================================================

void Logger::logSystemInit(const char* component, bool success) {
    if (success) {
        info("✅ %s inicializado correctamente", component);
    } else {
        error("❌ Error inicializando %s", component);
    }
}

void Logger::logGeofenceEvent(float distance, uint8_t alertLevel) {
    const char* levelNames[] = {"SAFE", "CAUTION", "WARNING", "DANGER", "EMERGENCY"};
    const char* emojis[] = {"✅", "⚠️", "🔸", "🔴", "🚨"};
    
    if (alertLevel < 5) {
        info("%s Geocerca - Distancia: %.1fm, Nivel: %s", 
             emojis[alertLevel], distance, levelNames[alertLevel]);
    }
}

void Logger::logPacketSent(uint16_t sequenceNumber, bool success) {
    if (success) {
        info("📡 Packet #%d enviado correctamente", sequenceNumber);
    } else {
        warn("📡 Error enviando packet #%d", sequenceNumber);
    }
}

void Logger::logBatteryStatus(float voltage, uint8_t percentage) {
    const char* emoji = "🔋";
    if (percentage < 20) emoji = "🪫";
    else if (percentage < 50) emoji = "🔋";
    else emoji = "🔋";
    
    info("%s Batería: %.2fV (%d%%)", emoji, voltage, percentage);
}

void Logger::logGPSPosition(double lat, double lng, bool valid) {
    if (valid) {
        info("📍 GPS: %.6f, %.6f", lat, lng);
    } else {
        warn("📍 GPS sin fix válido");
    }
}

void Logger::logMemoryStatus(uint32_t freeHeap) {
    debug("💾 Memoria libre: %lu bytes", freeHeap);
    
    if (freeHeap < 10000) {
        warn("💾 Memoria baja: %lu bytes", freeHeap);
    }
}

// ============================================================================
// BANNER Y INFORMACIÓN DEL SISTEMA
// ============================================================================

void Logger::printBanner() {
    if (!initialized) return;
    
    Serial.println();
    Serial.println("🚀 ===============================================");
    Serial.println("🐐 COLLAR GEOFENCING V3.0 - SISTEMA MODULAR");
    Serial.println("🚀 ===============================================");
    Serial.println("📱 Hardware: Heltec WiFi LoRa 32 V3");
    Serial.println("🧠 MCU: ESP32-S3FN8 @ 240MHz");
    Serial.println("📡 Radio: SX1262 LoRaWAN");
    Serial.println("📺 Display: OLED 128x64 I2C");
    Serial.println("🎵 Audio: Buzzer PWM optimizado");
    Serial.println("🎯 Firmware: " FIRMWARE_VERSION);
    Serial.println("🏭 Fabricante: " MANUFACTURER);
    Serial.println("🚀 ===============================================");
    Serial.println();
}

void Logger::printSystemInfo() {
    if (!initialized) return;
    
    info("🔧 Información del sistema:");
    info("   - Chip: %s", ESP.getChipModel());
    info("   - Revision: %d", ESP.getChipRevision());
    info("   - Cores: %d", ESP.getChipCores());
    info("   - CPU Freq: %d MHz", ESP.getCpuFreqMHz());
    info("   - Flash: %d KB", ESP.getFlashChipSize() / 1024);
    info("   - RAM Total: %d KB", ESP.getHeapSize() / 1024);
    info("   - RAM Libre: %d KB", ESP.getFreeHeap() / 1024);
    info("   - SDK: %s", ESP.getSdkVersion());
    
    // Información de reset
    esp_reset_reason_t resetReason = esp_reset_reason();
    const char* resetReasonStr = "UNKNOWN";
    switch (resetReason) {
        case ESP_RST_POWERON:   resetReasonStr = "POWER_ON"; break;
        case ESP_RST_EXT:       resetReasonStr = "EXTERNAL"; break;
        case ESP_RST_SW:        resetReasonStr = "SOFTWARE"; break;
        case ESP_RST_PANIC:     resetReasonStr = "PANIC"; break;
        case ESP_RST_INT_WDT:   resetReasonStr = "INT_WDT"; break;
        case ESP_RST_TASK_WDT:  resetReasonStr = "TASK_WDT"; break;
        case ESP_RST_WDT:       resetReasonStr = "WDT"; break;
        case ESP_RST_DEEPSLEEP: resetReasonStr = "DEEP_SLEEP"; break;
        case ESP_RST_BROWNOUT:  resetReasonStr = "BROWNOUT"; break;
        case ESP_RST_SDIO:      resetReasonStr = "SDIO"; break;
    }
    info("   - Reset: %s", resetReasonStr);
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

void Logger::logWithLevel(Level level, const char* prefix, const char* message, va_list args) {
    if (!initialized) return;
    
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), message, args);
    
    uint32_t uptime = getUptime();
    uint32_t hours = uptime / 3600;
    uint32_t minutes = (uptime % 3600) / 60;
    uint32_t seconds = uptime % 60;
    
    Serial.printf("[%02lu:%02lu:%02lu] %s [%s] %s\n", 
                  hours, minutes, seconds,
                  getLevelEmoji(level), prefix, buffer);
}

const char* Logger::getLevelString(Level level) {
    switch (level) {
        case ERROR: return "ERROR";
        case WARN:  return "WARN";
        case INFO:  return "INFO";
        case DEBUG: return "DEBUG";
        default:    return "UNKNOWN";
    }
}

const char* Logger::getLevelEmoji(Level level) {
    switch (level) {
        case ERROR: return "❌";
        case WARN:  return "⚠️";
        case INFO:  return "ℹ️";
        case DEBUG: return "🔍";
        default:    return "❓";
    }
}

uint32_t Logger::getUptime() {
    return initialized ? (millis() - bootTime) / 1000 : 0;
}
