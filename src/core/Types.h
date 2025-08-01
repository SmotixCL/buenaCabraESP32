#pragma once
#include <Arduino.h>

/*
 * ============================================================================
 * TIPOS DE DATOS COMUNES - COLLAR GEOFENCING
 * ============================================================================
 */

// ============================================================================
// POSICIÓN GPS
// ============================================================================
struct Position {
    double latitude;
    double longitude;
    float altitude;
    float accuracy;         // Precisión en metros
    uint8_t satellites;     // Número de satélites
    uint32_t timestamp;     // Timestamp de la lectura
    bool valid;             // Si la posición es válida
    
    Position() : 
        latitude(0.0), longitude(0.0), altitude(0.0), 
        accuracy(999.0f), satellites(0), timestamp(0), valid(false) {}
        
    Position(double lat, double lng) :
        latitude(lat), longitude(lng), altitude(0.0),
        accuracy(5.0f), satellites(4), timestamp(millis()), valid(true) {}
};

// ============================================================================
// GEOCERCA
// ============================================================================
struct Geofence {
    double centerLat;
    double centerLng;
    float radius;           // Radio en metros
    bool active;            // Si está activa
    uint32_t createdAt;     // Timestamp de creación
    char name[16];          // Nombre de la geocerca
    
    Geofence() :
        centerLat(0.0), centerLng(0.0), radius(50.0f), 
        active(false), createdAt(0) {
        strcpy(name, "Default");
    }
        
    Geofence(double lat, double lng, float r, const char* n = "Geofence") :
        centerLat(lat), centerLng(lng), radius(r), 
        active(true), createdAt(millis()) {
        strncpy(name, n, 15);
        name[15] = '\0';
    }
};

// ============================================================================
// ESTADO DE BATERÍA
// ============================================================================
struct BatteryStatus {
    float voltage;          // Voltaje actual
    uint8_t percentage;     // Porcentaje estimado (0-100)
    bool charging;          // Si está cargando
    bool low;               // Si está bajo
    bool critical;          // Si está crítico
    uint32_t lastReading;   // Último timestamp de lectura
    
    BatteryStatus() :
        voltage(0.0f), percentage(0), charging(false),
        low(false), critical(false), lastReading(0) {}
};

// ============================================================================
// ESTADO DEL SISTEMA
// ============================================================================
struct SystemStatus {
    bool radioInitialized;
    bool displayInitialized; 
    bool buzzerInitialized;
    bool gpsInitialized;
    uint8_t currentState;       // SystemState enum
    uint32_t uptime;            // Tiempo encendido (segundos)
    uint32_t freeHeap;          // Memoria libre
    float cpuTemperature;       // Temperatura CPU
    uint16_t resetCount;        // Contador de resets
    
    SystemStatus() :
        radioInitialized(false), displayInitialized(false),
        buzzerInitialized(false), gpsInitialized(false),
        currentState(0), uptime(0), freeHeap(0),
        cpuTemperature(0.0f), resetCount(0) {}
};

// ============================================================================
// PAQUETE DE DATOS LORAWAN
// ============================================================================
struct LoRaWANPacket {
    uint16_t sequenceNumber;
    Position position;
    uint8_t alertLevel;
    BatteryStatus battery;
    uint32_t timestamp;
    uint8_t payloadSize;
    uint8_t payload[32];        // Payload LoRaWAN
    
    LoRaWANPacket() :
        sequenceNumber(0), alertLevel(0), timestamp(0), payloadSize(0) {
        memset(payload, 0, sizeof(payload));
    }
};

// ============================================================================
// CONFIGURACIÓN DE ALERTA
// ============================================================================
struct AlertConfig {
    bool enabled;
    uint16_t frequency;         // Frecuencia del tono (Hz)
    uint16_t duration;          // Duración del tono (ms)
    uint8_t volume;             // Volumen (0-100)
    uint16_t interval;          // Intervalo entre repeticiones (ms)
    uint8_t repetitions;        // Número de repeticiones (0 = infinito)
    
    AlertConfig() :
        enabled(true), frequency(2730), duration(500),
        volume(75), interval(1000), repetitions(0) {}
        
    AlertConfig(uint16_t freq, uint16_t dur, uint8_t vol) :
        enabled(true), frequency(freq), duration(dur),
        volume(vol), interval(1000), repetitions(0) {}
};

// ============================================================================
// ESTADÍSTICAS DEL SISTEMA
// ============================================================================
struct SystemStats {
    uint32_t totalPacketsSent;
    uint32_t totalPacketsReceived;
    uint32_t packetsLost;
    uint32_t alertsTriggered;
    uint32_t geofenceViolations;
    uint32_t lowBatteryEvents;
    uint32_t totalUptime;       // Tiempo total de funcionamiento
    float averageBatteryVoltage;
    
    SystemStats() :
        totalPacketsSent(0), totalPacketsReceived(0), packetsLost(0),
        alertsTriggered(0), geofenceViolations(0), lowBatteryEvents(0),
        totalUptime(0), averageBatteryVoltage(0.0f) {}
};

// ============================================================================
// NIVELES DE ALERTA (enum class para type safety)
// ============================================================================
enum class AlertLevel : uint8_t {
    SAFE = 0,
    CAUTION = 1,
    WARNING = 2,
    DANGER = 3,
    EMERGENCY = 4
};

// ============================================================================
// RESULTADO DE OPERACIONES
// ============================================================================
enum class Result : uint8_t {
    SUCCESS = 0,
    ERROR_INIT = 1,
    ERROR_TIMEOUT = 2,
    ERROR_INVALID_PARAM = 3,
    ERROR_NO_MEMORY = 4,
    ERROR_HARDWARE = 5,
    ERROR_COMMUNICATION = 6,
    ERROR_GPS_NO_FIX = 7,
    ERROR_BATTERY_LOW = 8
};

// ============================================================================
// FUNCIONES DE UTILIDAD PARA TIPOS
// ============================================================================

// Convertir AlertLevel a string
inline const char* alertLevelToString(AlertLevel level) {
    switch (level) {
        case AlertLevel::SAFE:      return "SAFE";
        case AlertLevel::CAUTION:   return "CAUTION";
        case AlertLevel::WARNING:   return "WARNING";
        case AlertLevel::DANGER:    return "DANGER";
        case AlertLevel::EMERGENCY: return "EMERGENCY";
        default:                    return "UNKNOWN";
    }
}

// Convertir Result a string
inline const char* resultToString(Result result) {
    switch (result) {
        case Result::SUCCESS:               return "SUCCESS";
        case Result::ERROR_INIT:            return "ERROR_INIT";
        case Result::ERROR_TIMEOUT:         return "ERROR_TIMEOUT";
        case Result::ERROR_INVALID_PARAM:   return "ERROR_INVALID_PARAM";
        case Result::ERROR_NO_MEMORY:       return "ERROR_NO_MEMORY";
        case Result::ERROR_HARDWARE:        return "ERROR_HARDWARE";
        case Result::ERROR_COMMUNICATION:   return "ERROR_COMMUNICATION";
        case Result::ERROR_GPS_NO_FIX:      return "ERROR_GPS_NO_FIX";
        case Result::ERROR_BATTERY_LOW:     return "ERROR_BATTERY_LOW";
        default:                            return "UNKNOWN_ERROR";
    }
}

// Validar posición GPS
inline bool isValidPosition(const Position& pos) {
    return pos.valid && 
           pos.latitude >= -90.0 && pos.latitude <= 90.0 &&
           pos.longitude >= -180.0 && pos.longitude <= 180.0;
}

// Calcular porcentaje de batería basado en voltaje
inline uint8_t calculateBatteryPercentage(float voltage) {
    if (voltage >= 4.2f) return 100;
    if (voltage >= 4.0f) return 80 + (voltage - 4.0f) * 100;  // 80-100%
    if (voltage >= 3.8f) return 60 + (voltage - 3.8f) * 100;  // 60-80%
    if (voltage >= 3.6f) return 40 + (voltage - 3.6f) * 100;  // 40-60%
    if (voltage >= 3.4f) return 20 + (voltage - 3.4f) * 100;  // 20-40%
    if (voltage >= 3.0f) return 0 + (voltage - 3.0f) * 50;    // 0-20%
    return 0;
}
