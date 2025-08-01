#pragma once
#include <Arduino.h>
#include "config/constants.h"
#include "core/Types.h"
#include "core/Logger.h"

/*
 * ============================================================================
 * GEOFENCE MANAGER - GESTIÓN DE GEOCERCAS
 * ============================================================================
 */

class GeofenceManager {
public:
    GeofenceManager();
    
    // Inicialización
    Result init();
    bool isInitialized() const;
    
    // Gestión de geocerca principal
    void setGeofence(double centerLat, double centerLng, float radius, const char* name = "Principal");
    void setGeofence(const Geofence& geofence);
    Geofence getGeofence() const;
    
    // Control de activación
    void activate(bool enabled);
    bool isActive() const;
    
    // Verificación de posición
    bool isInsideGeofence(const Position& position) const;
    bool isInsideGeofence(double lat, double lng) const;
    float getDistance(const Position& position) const;
    float getDistance(double lat, double lng) const;
    
    // Información de la geocerca
    double getCenterLat() const;
    double getCenterLng() const;
    float getRadius() const;
    const char* getName() const;
    
    // Análisis y estadísticas
    AlertLevel calculateAlertLevel(const Position& position) const;
    AlertLevel calculateAlertLevel(float distance) const;
    uint32_t getViolationsCount() const;
    uint32_t getLastViolationTime() const;
    float getMinDistanceRecorded() const;
    
    // Múltiples geocercas (para futuras expansiones)
    static const uint8_t MAX_GEOFENCES = 5;
    Result addGeofence(const Geofence& geofence, uint8_t* index = nullptr);
    Result removeGeofence(uint8_t index);
    Result updateGeofence(uint8_t index, const Geofence& geofence);
    uint8_t getGeofenceCount() const;
    Geofence getGeofence(uint8_t index) const;
    
    // Verificación con múltiples geocercas
    bool isInsideAnyGeofence(const Position& position) const;
    float getMinDistance(const Position& position) const;
    AlertLevel getHighestAlertLevel(const Position& position) const;
    
    // Callbacks para eventos
    typedef void (*GeofenceCallback)(const Geofence& geofence, const Position& position, bool inside);
    typedef void (*ViolationCallback)(const Geofence& geofence, float distance, AlertLevel level);
    
    void setGeofenceCallback(GeofenceCallback callback);
    void setViolationCallback(ViolationCallback callback);
    
    // Update loop (llamar desde loop principal)
    void update(const Position& currentPosition);
    
    // Persistencia (guardar/cargar configuración)
    Result saveConfiguration();
    Result loadConfiguration();
    void resetToDefaults();
    
    // Utilidades de cálculo
    static float calculateDistance(double lat1, double lng1, double lat2, double lng2);
    static bool isValidCoordinate(double lat, double lng);
    
private:
    bool initialized;
    
    // Geocerca principal
    Geofence primaryGeofence;
    bool active;
    
    // Array de geocercas múltiples
    Geofence geofences[MAX_GEOFENCES];
    bool geofenceActive[MAX_GEOFENCES];
    uint8_t geofenceCount;
    
    // Estadísticas y estado
    uint32_t violationsCount;
    uint32_t lastViolationTime;
    float minDistanceRecorded;
    Position lastPosition;
    bool lastInsideState;
    AlertLevel lastAlertLevel;
    
    // Callbacks
    GeofenceCallback geofenceCallback;
    ViolationCallback violationCallback;
    
    // Configuración de alertas por distancia
    struct DistanceThreshold {
        float distance;
        AlertLevel level;
    };
    
    static const uint8_t THRESHOLD_COUNT = 4;
    DistanceThreshold thresholds[THRESHOLD_COUNT];
    
    // Métodos privados
    void initializeThresholds();
    void updateStatistics(const Position& position);
    void checkViolations(const Position& position);
    void triggerCallbacks(const Position& position);
    
    // Validación
    bool isValidGeofence(const Geofence& geofence) const;
    
    // Utilidades internas
    float distanceToGeofenceBoundary(const Geofence& geofence, double lat, double lng) const;
    bool isPositionInside(const Geofence& geofence, double lat, double lng) const;
    
    // Constantes para cálculos
    static constexpr double EARTH_RADIUS_M = 6371000.0; // Radio de la Tierra en metros
    // Usamos las definiciones de Arduino.h para DEG_TO_RAD y RAD_TO_DEG
};
