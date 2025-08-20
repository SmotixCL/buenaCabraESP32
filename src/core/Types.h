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
// GEOCERCA - SOPORTA CÍRCULOS Y POLÍGONOS
// ============================================================================

// Tipos de geocerca
enum class GeofenceType : uint8_t {
    CIRCLE = 0,
    POLYGON = 1
};

// Punto geográfico para polígonos
struct GeoPoint {
    double lat;
    double lng;
    
    GeoPoint() : lat(0.0), lng(0.0) {}
    GeoPoint(double latitude, double longitude) : lat(latitude), lng(longitude) {}
};

// Estructura principal de geocerca
struct Geofence {
    GeofenceType type;          // Tipo: círculo o polígono
    bool active;                // Si está activa
    uint32_t createdAt;         // Timestamp de creación
    char name[16];              // Nombre de la geocerca
    char groupId[16];           // ID del grupo asignado
    
    // Para círculos
    double centerLat;
    double centerLng;
    float radius;               // Radio en metros
    
    // Para polígonos
    static const uint8_t MAX_POLYGON_POINTS = 10;
    GeoPoint points[MAX_POLYGON_POINTS];
    uint8_t pointCount;         // Número de puntos del polígono
    
    Geofence() :
        type(GeofenceType::CIRCLE),
        active(false), createdAt(0),
        centerLat(0.0), centerLng(0.0), radius(50.0f),
        pointCount(0) {
        strcpy(name, "Default");
        strcpy(groupId, "none");
        // Inicializar array de puntos
        for(uint8_t i = 0; i < MAX_POLYGON_POINTS; i++) {
            points[i] = GeoPoint();
        }
    }
        
    // Constructor para círculo
    Geofence(double lat, double lng, float r, const char* n = "Geofence", const char* group = "none") :
        type(GeofenceType::CIRCLE),
        active(true), createdAt(millis()),
        centerLat(lat), centerLng(lng), radius(r),
        pointCount(0) {
        strncpy(name, n, 15);
        name[15] = '\0';
        strncpy(groupId, group, 15);
        groupId[15] = '\0';
        for(uint8_t i = 0; i < MAX_POLYGON_POINTS; i++) {
            points[i] = GeoPoint();
        }
    }
    
    // Constructor para polígono
    Geofence(const GeoPoint* polygonPoints, uint8_t numPoints, const char* n = "Polygon", const char* group = "none") :
        type(GeofenceType::POLYGON),
        active(true), createdAt(millis()),
        centerLat(0.0), centerLng(0.0), radius(0.0f),
        pointCount(numPoints > MAX_POLYGON_POINTS ? MAX_POLYGON_POINTS : numPoints) {
        strncpy(name, n, 15);
        name[15] = '\0';
        strncpy(groupId, group, 15);
        groupId[15] = '\0';
        
        // Copiar puntos del polígono
        for(uint8_t i = 0; i < pointCount; i++) {
            points[i] = polygonPoints[i];
        }
        
        // Calcular centro aproximado del polígono
        if(pointCount > 0) {
            double sumLat = 0.0, sumLng = 0.0;
            for(uint8_t i = 0; i < pointCount; i++) {
                sumLat += points[i].lat;
                sumLng += points[i].lng;
            }
            centerLat = sumLat / pointCount;
            centerLng = sumLng / pointCount;
        }
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
// PAQUETE DE DATOS LORAWAN MEJORADO
// ============================================================================
struct LoRaWANPacket {
    uint16_t sequenceNumber;
    Position position;
    uint8_t alertLevel;
    BatteryStatus battery;
    uint32_t timestamp;
    uint8_t payloadSize;
    uint8_t payload[32];        // Payload LoRaWAN
    
    // NUEVO: Estado del dispositivo
    char currentGroupId[16];    // Grupo asignado actualmente
    bool hasActiveGeofence;     // Si tiene geocerca activa
    GeofenceType geofenceType;  // Tipo de geocerca activa
    uint8_t deviceStatus;       // Bits de estado del dispositivo
    
    LoRaWANPacket() :
        sequenceNumber(0), alertLevel(0), timestamp(0), payloadSize(0),
        hasActiveGeofence(false), geofenceType(GeofenceType::CIRCLE), deviceStatus(0) {
        memset(payload, 0, sizeof(payload));
        strcpy(currentGroupId, "none");
    }
};

// ============================================================================
// PAYLOAD OPTIMIZADO PARA UPLINKS (11 bytes total)
// ============================================================================
struct GPSPayloadV2 {
    int32_t latitude;       // 4 bytes - lat * 10^7
    int32_t longitude;      // 4 bytes - lng * 10^7
    uint16_t altitude;      // 2 bytes - metros
    uint8_t satellites;     // 1 byte - número de satélites
    uint8_t hdop;           // 1 byte - HDOP * 10
    uint8_t battery;        // 1 byte - porcentaje batería
    uint8_t alert;          // 1 byte - nivel de alerta
    uint8_t status;         // 1 byte - flags de estado
    uint8_t groupIdHash;    // 1 byte - hash del grupo asignado
    uint8_t geofenceFlags;  // 1 byte - tipo geocerca + activa + dentro
    uint8_t frameCounter;   // 1 byte - contador para tracking
} __attribute__((packed));

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
// FLAGS PARA ESTADO DEL DISPOSITIVO
// ============================================================================
#define GEOFENCE_TYPE_MASK      0x03  // bits 0-1: tipo (0=círculo, 1=polígono)
#define GEOFENCE_ACTIVE_FLAG    0x04  // bit 2: geocerca activa
#define GEOFENCE_INSIDE_FLAG    0x08  // bit 3: dentro de geocerca
#define GEOFENCE_VIOLATION_FLAG 0x10  // bit 4: violación detectada
#define DEVICE_GPS_FIX_FLAG     0x20  // bit 5: GPS fix válido
#define DEVICE_BATTERY_LOW_FLAG 0x40  // bit 6: batería baja
#define DEVICE_ERROR_FLAG       0x80  // bit 7: error del sistema

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

// Convertir GeofenceType a string
inline const char* geofenceTypeToString(GeofenceType type) {
    switch (type) {
        case GeofenceType::CIRCLE:  return "CIRCLE";
        case GeofenceType::POLYGON: return "POLYGON";
        default:                    return "UNKNOWN";
    }
}

// Hash simple para nombres de grupo (8 bits)
inline uint8_t calculateGroupHash(const char* groupId) {
    uint8_t hash = 0;
    for (int i = 0; groupId[i] != '\0' && i < 15; i++) {
        hash = (hash * 31 + groupId[i]) & 0xFF;
    }
    return hash;
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

// Crear payload optimizado con estado del dispositivo
inline void createDeviceStatusPayload(GPSPayloadV2* payload, const Position& pos, 
                                     const BatteryStatus& battery, AlertLevel alert,
                                     const Geofence& geofence, bool gpsValid, 
                                     bool insideGeofence, uint8_t frameCount) {
    payload->latitude = (int32_t)(pos.latitude * 10000000);
    payload->longitude = (int32_t)(pos.longitude * 10000000);
    payload->altitude = (uint16_t)pos.altitude;
    payload->satellites = pos.satellites;
    payload->hdop = (uint8_t)(pos.accuracy * 10); // Usar accuracy como HDOP
    payload->battery = battery.percentage;
    payload->alert = (uint8_t)alert;
    
    // Flags de estado del dispositivo
    payload->status = 0;
    if (gpsValid) payload->status |= DEVICE_GPS_FIX_FLAG;
    if (battery.low) payload->status |= DEVICE_BATTERY_LOW_FLAG;
    if (insideGeofence) payload->status |= GEOFENCE_INSIDE_FLAG;
    
    // Hash del grupo
    payload->groupIdHash = calculateGroupHash(geofence.groupId);
    
    // Flags de geocerca
    payload->geofenceFlags = 0;
    payload->geofenceFlags |= (uint8_t)geofence.type & GEOFENCE_TYPE_MASK;
    if (geofence.active) payload->geofenceFlags |= GEOFENCE_ACTIVE_FLAG;
    if (insideGeofence) payload->geofenceFlags |= GEOFENCE_INSIDE_FLAG;
    
    // Frame counter para tracking
    payload->frameCounter = frameCount;
}
