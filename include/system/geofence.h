/*
 * ============================================================================
 * SISTEMA DE GEOFENCING - Gestión de Geocercas
 * ============================================================================
 * 
 * Sistema completo de geofencing con detección de zona, cálculo de distancias
 * y gestión de múltiples geocercas.
 * ============================================================================
 */

#ifndef GEOFENCE_H
#define GEOFENCE_H

#include "config.h"
#include <math.h>

// ============================================================================
// CONSTANTES GEOFENCING
// ============================================================================
#define MAX_GEOFENCES           5       // Máximo 5 geocercas simultáneas
#define EARTH_RADIUS_M          6371000 // Radio de la Tierra en metros
#define DEG_TO_RAD              (PI / 180.0)
#define RAD_TO_DEG              (180.0 / PI)

// ============================================================================
// ENUMERACIONES
// ============================================================================
enum GeofenceStatus {
    GEO_STATUS_UNKNOWN = 0,
    GEO_STATUS_INSIDE,
    GEO_STATUS_OUTSIDE,
    GEO_STATUS_ENTERING,
    GEO_STATUS_EXITING
};

enum GeofenceShape {
    GEO_SHAPE_CIRCLE = 0,
    GEO_SHAPE_RECTANGLE,
    GEO_SHAPE_POLYGON
};

// ============================================================================
// ESTRUCTURAS
// ============================================================================
typedef struct {
    double lat;
    double lng;
} GeoPoint_t;

typedef struct {
    uint8_t id;
    char name[32];
    bool active;
    GeofenceShape shape;
    
    // Para círculos
    GeoPoint_t center;
    float radius;
    
    // Para rectángulos
    GeoPoint_t top_left;
    GeoPoint_t bottom_right;
    
    // Estado actual
    GeofenceStatus status;
    float distance_to_boundary;
    uint32_t last_check;
    uint32_t time_inside;
    uint32_t time_outside;
    
} GeofenceZone_t;

// ============================================================================
// CLASE GEOFENCE MANAGER
// ============================================================================
class GeofenceManager {
private:
    GeofenceZone_t geofences[MAX_GEOFENCES];
    uint8_t active_count;
    GeoPoint_t last_position;
    bool position_valid;
    uint32_t last_global_check;
    
    // Métodos de cálculo
    float calculateDistance(const GeoPoint_t& point1, const GeoPoint_t& point2);
    float calculateBearing(const GeoPoint_t& from, const GeoPoint_t& to);
    bool isInsideCircle(const GeoPoint_t& point, const GeofenceZone_t& geofence);
    bool isInsideRectangle(const GeoPoint_t& point, const GeofenceZone_t& geofence);
    float distanceToCircleBoundary(const GeoPoint_t& point, const GeofenceZone_t& geofence);
    float distanceToRectangleBoundary(const GeoPoint_t& point, const GeofenceZone_t& geofence);
    
    // Gestión de estados
    void updateGeofenceStatus(GeofenceZone_t& geofence, const GeoPoint_t& current_pos);
    GeofenceStatus determineStatus(bool currently_inside, bool previously_inside);
    
public:
    GeofenceManager();
    ~GeofenceManager();
    
    // Inicialización
    void begin();
    void end();
    
    // Gestión de geocercas
    uint8_t addCircleGeofence(const char* name, double center_lat, double center_lng, float radius);
    uint8_t addRectangleGeofence(const char* name, double top_left_lat, double top_left_lng, 
                                double bottom_right_lat, double bottom_right_lng);
    bool removeGeofence(uint8_t id);
    bool activateGeofence(uint8_t id, bool active = true);
    void clearAllGeofences();
    
    // Verificación y estado
    void updatePosition(double latitude, double longitude);
    void checkAllGeofences();
    bool isAnyGeofenceViolated();
    GeofenceZone_t* getMostCriticalGeofence();
    uint8_t getViolatedGeofencesCount();
    
    // Consultas
    GeofenceZone_t* getGeofence(uint8_t id);
    uint8_t getActiveCount() const { return active_count; }
    float getClosestBoundaryDistance();
    GeofenceStatus getOverallStatus();
    
    // Configuración por defecto
    void setupDefaultGeofence(double center_lat, double center_lng, float radius = DEFAULT_GEOFENCE_RADIUS);
    
    // Información y debugging
    void printGeofenceInfo(uint8_t id);
    void printAllGeofences();
    String getStatusString();
    
    // Test y simulación
    void runSelfTest();
    void simulateMovement(double start_lat, double start_lng, double end_lat, double end_lng, uint8_t steps = 10);
};

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
extern GeofenceManager Geofence;

#endif // GEOFENCE_H