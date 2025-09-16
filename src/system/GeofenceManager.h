#pragma once
#include <Arduino.h>
#include "../config/pins.h"
#include "../config/constants.h"
#include "../core/Types.h"

/*
 * ============================================================================
 * GEOFENCE MANAGER - GESTIÓN DE GEOCERCAS CON SOPORTE POLÍGONOS
 * ============================================================================
 */

class GeofenceManager
{
public:
    GeofenceManager();

    // Inicialización
    Result init();
    bool isInitialized() const;

    // Gestión de geocerca principal
    void setGeofence(double centerLat, double centerLng, float radius, const char *name = "Principal", const char *groupId = "none");
    void setGeofence(const Geofence &geofence);
    void setPolygonGeofence(const GeoPoint *points, uint8_t numPoints, const char *name = "Polygon", const char *groupId = "none");
    Geofence getGeofence() const;

    // Control de activación
    void activate(bool enabled);
    bool isActive() const;

    // Verificación de posición - círculos y polígonos
    bool isInsideGeofence(const Position &position) const;
    bool isInsideGeofence(double lat, double lng) const;
    float getDistance(const Position &position) const;
    float getDistance(double lat, double lng) const;

    // Información de la geocerca
    double getCenterLat() const;
    double getCenterLng() const;
    float getRadius() const;
    const char *getName() const;
    const char *getGroupId() const;
    GeofenceType getType() const;

    // Para polígonos
    uint8_t getPolygonPointCount() const;
    GeoPoint getPolygonPoint(uint8_t index) const;
    bool hasValidPolygon() const;

    // Análisis y estadísticas
    AlertLevel calculateAlertLevel(const Position &position) const;
    AlertLevel calculateAlertLevel(float distance) const;
    uint32_t getViolationsCount() const;
    uint32_t getLastViolationTime() const;
    float getMinDistanceRecorded() const;

    // Múltiples geocercas (para futuras expansiones)
    static const uint8_t MAX_GEOFENCES = 5;
    Result addGeofence(const Geofence &geofence, uint8_t *index = nullptr);
    Result removeGeofence(uint8_t index);
    Result updateGeofence(uint8_t index, const Geofence &geofence);
    uint8_t getGeofenceCount() const;
    Geofence getGeofence(uint8_t index) const;

    // Verificación con múltiples geocercas
    bool isInsideAnyGeofence(const Position &position) const;
    float getMinDistance(const Position &position) const;
    AlertLevel getHighestAlertLevel(const Position &position) const;

    // Callbacks para eventos
    typedef void (*GeofenceCallback)(const Geofence &geofence, const Position &position, bool inside);
    typedef void (*ViolationCallback)(const Geofence &geofence, float distance, AlertLevel level);

    void setGeofenceCallback(GeofenceCallback callback);
    void setViolationCallback(ViolationCallback callback);

    // Update loop (llamar desde loop principal)
    void update(const Position &currentPosition);

    // ❌ PERSISTENCIA REMOVIDA POR SEGURIDAD
    // NO HAY persistencia automática de geocercas por temas de seguridad
    // Las geocercas deben ser reconfiguradas desde el backend después de cada reinicio
    void clearCurrentGeofence();
    void resetToDefaults();

    // Utilidades de cálculo - estáticas
    static float calculateDistance(double lat1, double lng1, double lat2, double lng2);
    static bool isValidCoordinate(double lat, double lng);

    // NUEVO: Algoritmos para polígonos
    static bool isPointInPolygon(double lat, double lng, const GeoPoint *points, uint8_t numPoints);
    static float distanceToPolygonBoundary(double lat, double lng, const GeoPoint *points, uint8_t numPoints);
    static float distanceToLineSegment(double lat, double lng, const GeoPoint &p1, const GeoPoint &p2);

private:
    bool initialized;

    // Geocerca principal (solo una activa a la vez por seguridad)
    Geofence primaryGeofence;
    bool active;

    // Array de geocercas múltiples (para expansión futura)
    Geofence geofences[MAX_GEOFENCES];
    bool geofenceActive[MAX_GEOFENCES];
    uint8_t geofenceCount;

    // Estadísticas y estado (NO persistentes)
    uint32_t violationsCount;
    uint32_t lastViolationTime;
    float minDistanceRecorded;
    Position lastPosition;
    bool lastInsideState;

    // Callbacks
    GeofenceCallback geofenceCallback;
    ViolationCallback violationCallback;

    // Configuración de alertas por distancia
    struct DistanceThreshold
    {
        float distance;
        AlertLevel level;
    };

    // Métodos privados
    void updateStatistics(const Position &position);
    void checkViolations(const Position &position);
    void triggerCallbacks(const Position &position);

    // Validación
    bool isValidGeofence(const Geofence &geofence) const;
    bool isValidPolygonGeofence(const GeoPoint *points, uint8_t numPoints) const;

    // Utilidades internas - círculos
    float distanceToCircleBoundary(const Geofence &geofence, double lat, double lng) const;
    bool isPositionInsideCircle(const Geofence &geofence, double lat, double lng) const;

    // Utilidades internas - polígonos
    float distanceToPolygonBoundaryInternal(const Geofence &geofence, double lat, double lng) const;
    bool isPositionInsidePolygon(const Geofence &geofence, double lat, double lng) const;

    // Constantes para cálculos
    static constexpr double EARTH_RADIUS_M = 6371000.0; // Radio de la Tierra en metros
    static constexpr double MIN_POLYGON_AREA = 100.0;   // Área mínima para polígonos (m²)
    // Usamos las definiciones de Arduino.h para DEG_TO_RAD y RAD_TO_DEG
};
