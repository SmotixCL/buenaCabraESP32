#pragma once
#include <Arduino.h>
#include <string.h>

// --- Enum de Resultado ---
enum class Result : uint8_t
{
    SUCCESS = 0,
    ERROR_INIT,
    ERROR_TIMEOUT,
    ERROR_INVALID_PARAM,
    ERROR_NO_MEMORY,
    ERROR_HARDWARE,
    ERROR_COMMUNICATION,
    ERROR_GPS_NO_FIX,
    ERROR_BATTERY_LOW
};

// --- Enum de Nivel de Alerta ---
enum class AlertLevel : uint8_t
{
    SAFE = 0,
    CAUTION,
    WARNING
};

/**
 * @TO DO
 */
// --- Tipos de Geocerca ---
enum class GeofenceType : uint8_t
{
    CIRCLE = 0,
    POLYGON // REVISAR SI ES NECESARIO AÑADIR OTRO TIPO PARA COMPRESSED POLYGON
};

// --- Estructuras de Datos ---
struct Position
{
    double latitude = 0.0;
    double longitude = 0.0;
    float altitude = 0.0;
    float accuracy = 99.0;
    uint8_t satellites = 0;
    uint32_t timestamp = 0;
    bool valid = false;
};
struct BatteryStatus
{
    float voltage = 0.0;
    uint8_t percentage = 0;
    bool charging = false;
    bool low = false;
    bool critical = false;
    uint32_t lastReading = 0;
};
struct SystemStatus
{
    bool buzzerInitialized, displayInitialized, gpsInitialized, radioInitialized;
    uint32_t uptime;
    uint32_t freeHeap;
    uint8_t currentState;
};
struct SystemStats
{
    uint32_t totalPacketsSent, totalPacketsReceived, packetsLost, alertsTriggered,
        geofenceViolations, lowBatteryEvents, totalUptime;
    float averageBatteryVoltage;
    // Campos adicionales para compatibilidad
    uint32_t successfulPackets;
    uint32_t failedPackets;
    int16_t lastRSSI;
    float lastSNR;
};
struct GeoPoint
{
    double lat;
    double lng;
    GeoPoint(double latitude = 0.0, double longitude = 0.0) : lat(latitude), lng(longitude) {}
};
struct Geofence
{
    GeofenceType type;
    bool active;
    bool isConfigured;
    char name[32];
    char groupId[16];
    double centerLat;
    double centerLng;
    float radius;
    static const uint8_t MAX_POLYGON_POINTS = 10;
    GeoPoint points[MAX_POLYGON_POINTS];
    uint8_t pointCount;

    // Constructores
    Geofence() : type(GeofenceType::CIRCLE), active(false), isConfigured(false),
                 centerLat(0.0), centerLng(0.0), radius(0.0), pointCount(0)
    {
        strcpy(name, "Default");
        strcpy(groupId, "none");
    }

    // Constructor para círculo
    Geofence(double lat, double lng, float r, const char *n, const char *gid)
        : type(GeofenceType::CIRCLE), active(true), isConfigured(true),
          centerLat(lat), centerLng(lng), radius(r), pointCount(0)
    {
        strncpy(name, n, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        strncpy(groupId, gid, sizeof(groupId) - 1);
        groupId[sizeof(groupId) - 1] = '\0';
    }

    // Constructor para polígono
    Geofence(const GeoPoint *pts, uint8_t count, const char *n, const char *gid)
        : type(GeofenceType::POLYGON), active(true), isConfigured(true),
          centerLat(0.0), centerLng(0.0), radius(0.0), pointCount(count)
    {
        strncpy(name, n, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        strncpy(groupId, gid, sizeof(groupId) - 1);
        groupId[sizeof(groupId) - 1] = '\0';

        // Copiar puntos y calcular centro
        if (pts && count > 0 && count <= MAX_POLYGON_POINTS)
        {
            double sumLat = 0, sumLng = 0;
            for (uint8_t i = 0; i < count; i++)
            {
                points[i] = pts[i];
                sumLat += pts[i].lat;
                sumLng += pts[i].lng;
            }
            centerLat = sumLat / count;
            centerLng = sumLng / count;
        }
    }
};
// GeofenceUpdate está definido en lorawan_config.h
struct AlertConfig
{
    bool enabled;
    uint16_t frequency;
    uint16_t duration;
    uint8_t volume;
    uint16_t interval;
    uint8_t repetitions;

    // Constructor para inicializar fácilmente
    AlertConfig(uint16_t freq = 0, uint16_t dur = 0, uint8_t vol = 0)
        : enabled(true), frequency(freq), duration(dur), volume(vol), interval(1000), repetitions(0) {}
};

// --- Funciones de ayuda ---
inline const char *alertLevelToString(AlertLevel level)
{
    switch (level)
    {
    case AlertLevel::SAFE:
        return "SAFE";
    case AlertLevel::CAUTION:
        return "CAUTION";
    case AlertLevel::WARNING:
        return "WARNING";
    default:
        return "UNKNOWN";
    }
}

inline const char *geofenceTypeToString(GeofenceType type)
{
    switch (type)
    {
    case GeofenceType::CIRCLE:
        return "CIRCLE";
    case GeofenceType::POLYGON:
        return "POLYGON";
    default:
        return "UNKNOWN";
    }
}

inline bool isValidPosition(const Position &pos)
{
    return pos.valid &&
           pos.latitude >= -90.0 && pos.latitude <= 90.0 &&
           pos.longitude >= -180.0 && pos.longitude <= 180.0;
}