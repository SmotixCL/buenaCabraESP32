/*
 * ============================================================================
 * IMPLEMENTACIÓN SISTEMA DE GEOFENCING - Gestión de Geocercas
 * ============================================================================
 */

#include "system/geofence.h"

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
GeofenceManager Geofence;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================
GeofenceManager::GeofenceManager() 
    : active_count(0), position_valid(false), last_global_check(0) {
    
    // Inicializar todas las geocercas como inactivas
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        geofences[i].id = i;
        geofences[i].active = false;
        geofences[i].status = GEO_STATUS_UNKNOWN;
        strcpy(geofences[i].name, "");
    }
    
    last_position.lat = 0.0;
    last_position.lng = 0.0;
}

GeofenceManager::~GeofenceManager() {
    end();
}

// ============================================================================
// INICIALIZACIÓN
// ============================================================================
void GeofenceManager::begin() {
    DEBUG_PRINTLN(F("🎯 Inicializando sistema de geofencing..."));
    
    clearAllGeofences();
    position_valid = false;
    last_global_check = millis();
    
    DEBUG_PRINTLN(F("✅ Sistema de geofencing inicializado"));
}

void GeofenceManager::end() {
    clearAllGeofences();
    position_valid = false;
    DEBUG_PRINTLN(F("🎯 Sistema de geofencing desactivado"));
}

// ============================================================================
// CÁLCULOS GEOGRÁFICOS
// ============================================================================
float GeofenceManager::calculateDistance(const GeoPoint_t& point1, const GeoPoint_t& point2) {
    // Fórmula de Haversine para cálculo preciso de distancias
    double lat1_rad = point1.lat * DEG_TO_RAD;
    double lat2_rad = point2.lat * DEG_TO_RAD;
    double dlat_rad = (point2.lat - point1.lat) * DEG_TO_RAD;
    double dlng_rad = (point2.lng - point1.lng) * DEG_TO_RAD;
    
    double a = sin(dlat_rad / 2) * sin(dlat_rad / 2) +
              cos(lat1_rad) * cos(lat2_rad) *
              sin(dlng_rad / 2) * sin(dlng_rad / 2);
    
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return EARTH_RADIUS_M * c;
}

float GeofenceManager::calculateBearing(const GeoPoint_t& from, const GeoPoint_t& to) {
    double lat1_rad = from.lat * DEG_TO_RAD;
    double lat2_rad = to.lat * DEG_TO_RAD;
    double dlng_rad = (to.lng - from.lng) * DEG_TO_RAD;
    
    double y = sin(dlng_rad) * cos(lat2_rad);
    double x = cos(lat1_rad) * sin(lat2_rad) - 
              sin(lat1_rad) * cos(lat2_rad) * cos(dlng_rad);
    
    double bearing_rad = atan2(y, x);
    return fmod((bearing_rad * RAD_TO_DEG + 360.0), 360.0);
}

bool GeofenceManager::isInsideCircle(const GeoPoint_t& point, const GeofenceZone_t& geofence) {
    float distance = calculateDistance(point, geofence.center);
    return distance <= geofence.radius;
}

bool GeofenceManager::isInsideRectangle(const GeoPoint_t& point, const GeofenceZone_t& geofence) {
    return (point.lat <= geofence.top_left.lat &&
            point.lat >= geofence.bottom_right.lat &&
            point.lng >= geofence.top_left.lng &&
            point.lng <= geofence.bottom_right.lng);
}

float GeofenceManager::distanceToCircleBoundary(const GeoPoint_t& point, const GeofenceZone_t& geofence) {
    float distance_to_center = calculateDistance(point, geofence.center);
    return geofence.radius - distance_to_center; // Positivo = dentro, Negativo = fuera
}

float GeofenceManager::distanceToRectangleBoundary(const GeoPoint_t& point, const GeofenceZone_t& geofence) {
    // Simplificación: distancia al borde más cercano
    float dist_to_top = (geofence.top_left.lat - point.lat) * 111000.0;
    float dist_to_bottom = (point.lat - geofence.bottom_right.lat) * 111000.0;
    float dist_to_left = (point.lng - geofence.top_left.lng) * 111000.0;
    float dist_to_right = (geofence.bottom_right.lng - point.lng) * 111000.0;
    
    if (isInsideRectangle(point, geofence)) {
        // Dentro: distancia positiva al borde más cercano
        return min(min(dist_to_top, dist_to_bottom), min(dist_to_left, dist_to_right));
    } else {
        // Fuera: distancia negativa al borde más cercano
        float min_distance = 0;
        if (point.lat > geofence.top_left.lat) min_distance = max(min_distance, dist_to_top);
        if (point.lat < geofence.bottom_right.lat) min_distance = max(min_distance, abs(dist_to_bottom));
        if (point.lng < geofence.top_left.lng) min_distance = max(min_distance, abs(dist_to_left));
        if (point.lng > geofence.bottom_right.lng) min_distance = max(min_distance, dist_to_right);
        return -min_distance;
    }
}

// ============================================================================
// GESTIÓN DE GEOCERCAS
// ============================================================================
uint8_t GeofenceManager::addCircleGeofence(const char* name, double center_lat, double center_lng, float radius) {
    if (active_count >= MAX_GEOFENCES) {
        DEBUG_PRINTLN(F("❌ Error: Máximo número de geocercas alcanzado"));
        return 255; // ID inválido
    }
    
    // Buscar slot libre
    uint8_t slot = 255;
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        if (!geofences[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == 255) {
        DEBUG_PRINTLN(F("❌ Error: No hay slots libres para geocerca"));
        return 255;
    }
    
    // Configurar geocerca
    GeofenceZone_t& geo = geofences[slot];
    geo.id = slot;
    strncpy(geo.name, name, sizeof(geo.name) - 1);
    geo.name[sizeof(geo.name) - 1] = '\0';
    geo.active = true;
    geo.shape = GEO_SHAPE_CIRCLE;
    geo.center.lat = center_lat;
    geo.center.lng = center_lng;
    geo.radius = radius;
    geo.status = GEO_STATUS_UNKNOWN;
    geo.distance_to_boundary = 999.0;
    geo.last_check = 0;
    geo.time_inside = 0;
    geo.time_outside = 0;
    
    active_count++;
    
    DEBUG_PRINTF("✅ Geocerca circular creada: ID=%d, '%s', %.6f,%.6f, R=%.1fm\n", 
                 slot, name, center_lat, center_lng, radius);
    
    return slot;
}

uint8_t GeofenceManager::addRectangleGeofence(const char* name, double top_left_lat, double top_left_lng, 
                                            double bottom_right_lat, double bottom_right_lng) {
    if (active_count >= MAX_GEOFENCES) {
        DEBUG_PRINTLN(F("❌ Error: Máximo número de geocercas alcanzado"));
        return 255;
    }
    
    // Buscar slot libre
    uint8_t slot = 255;
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        if (!geofences[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == 255) return 255;
    
    // Configurar geocerca rectangular
    GeofenceZone_t& geo = geofences[slot];
    geo.id = slot;
    strncpy(geo.name, name, sizeof(geo.name) - 1);
    geo.name[sizeof(geo.name) - 1] = '\0';
    geo.active = true;
    geo.shape = GEO_SHAPE_RECTANGLE;
    geo.top_left.lat = top_left_lat;
    geo.top_left.lng = top_left_lng;
    geo.bottom_right.lat = bottom_right_lat;
    geo.bottom_right.lng = bottom_right_lng;
    geo.status = GEO_STATUS_UNKNOWN;
    geo.distance_to_boundary = 999.0;
    geo.last_check = 0;
    geo.time_inside = 0;
    geo.time_outside = 0;
    
    active_count++;
    
    DEBUG_PRINTF("✅ Geocerca rectangular creada: ID=%d, '%s'\n", slot, name);
    
    return slot;
}

bool GeofenceManager::removeGeofence(uint8_t id) {
    if (id >= MAX_GEOFENCES || !geofences[id].active) {
        return false;
    }
    
    geofences[id].active = false;
    geofences[id].status = GEO_STATUS_UNKNOWN;
    strcpy(geofences[id].name, "");
    active_count--;
    
    DEBUG_PRINTF("🗑️ Geocerca eliminada: ID=%d\n", id);
    return true;
}

bool GeofenceManager::activateGeofence(uint8_t id, bool active) {
    if (id >= MAX_GEOFENCES) return false;
    
    if (geofences[id].active != active) {
        geofences[id].active = active;
        if (active) {
            active_count++;
        } else {
            active_count--;
        }
        
        DEBUG_PRINTF("🎯 Geocerca ID=%d %s\n", id, active ? "activada" : "desactivada");
    }
    
    return true;
}

void GeofenceManager::clearAllGeofences() {
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        geofences[i].active = false;
        geofences[i].status = GEO_STATUS_UNKNOWN;
        strcpy(geofences[i].name, "");
    }
    active_count = 0;
    DEBUG_PRINTLN(F("🧹 Todas las geocercas eliminadas"));
}

// ============================================================================
// VERIFICACIÓN Y ESTADO
// ============================================================================
void GeofenceManager::updatePosition(double latitude, double longitude) {
    last_position.lat = latitude;
    last_position.lng = longitude;
    position_valid = true;
    
    DEBUG_PRINTF("📍 Posición actualizada: %.6f, %.6f\n", latitude, longitude);
}

void GeofenceManager::updateGeofenceStatus(GeofenceZone_t& geofence, const GeoPoint_t& current_pos) {
    uint32_t current_time = millis();
    bool currently_inside = false;
    float distance_to_boundary = 999.0;
    
    // Determinar si está dentro según el tipo de geocerca
    switch (geofence.shape) {
        case GEO_SHAPE_CIRCLE:
            currently_inside = isInsideCircle(current_pos, geofence);
            distance_to_boundary = distanceToCircleBoundary(current_pos, geofence);
            break;
            
        case GEO_SHAPE_RECTANGLE:
            currently_inside = isInsideRectangle(current_pos, geofence);
            distance_to_boundary = distanceToRectangleBoundary(current_pos, geofence);
            break;
            
        default:
            return; // Tipo no soportado
    }
    
    // Determinar estado basado en historia
    bool previously_inside = (geofence.status == GEO_STATUS_INSIDE || 
                             geofence.status == GEO_STATUS_ENTERING);
    
    GeofenceStatus new_status = determineStatus(currently_inside, previously_inside);
    
    // Actualizar tiempos
    if (geofence.last_check > 0) {
        uint32_t time_delta = current_time - geofence.last_check;
        if (currently_inside) {
            geofence.time_inside += time_delta;
        } else {
            geofence.time_outside += time_delta;
        }
    }
    
    // Log de cambios de estado
    if (new_status != geofence.status) {
        DEBUG_PRINTF("🎯 Geocerca '%s': %s -> %s (dist: %.1fm)\n", 
                     geofence.name,
                     geofence.status == GEO_STATUS_INSIDE ? "INSIDE" : 
                     geofence.status == GEO_STATUS_OUTSIDE ? "OUTSIDE" : "UNKNOWN",
                     new_status == GEO_STATUS_INSIDE ? "INSIDE" : 
                     new_status == GEO_STATUS_OUTSIDE ? "OUTSIDE" : 
                     new_status == GEO_STATUS_ENTERING ? "ENTERING" : "EXITING",
                     distance_to_boundary);
    }
    
    // Actualizar estado
    geofence.status = new_status;
    geofence.distance_to_boundary = distance_to_boundary;
    geofence.last_check = current_time;
}

GeofenceStatus GeofenceManager::determineStatus(bool currently_inside, bool previously_inside) {
    if (currently_inside && previously_inside) {
        return GEO_STATUS_INSIDE;
    } else if (!currently_inside && !previously_inside) {
        return GEO_STATUS_OUTSIDE;
    } else if (currently_inside && !previously_inside) {
        return GEO_STATUS_ENTERING;
    } else { // !currently_inside && previously_inside
        return GEO_STATUS_EXITING;
    }
}

void GeofenceManager::checkAllGeofences() {
    if (!position_valid || active_count == 0) return;
    
    uint32_t current_time = millis();
    if (current_time - last_global_check < GEOFENCE_CHECK_INTERVAL) return;
    
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        if (geofences[i].active) {
            updateGeofenceStatus(geofences[i], last_position);
        }
    }
    
    last_global_check = current_time;
}

bool GeofenceManager::isAnyGeofenceViolated() {
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        if (geofences[i].active && 
            (geofences[i].status == GEO_STATUS_OUTSIDE || 
             geofences[i].status == GEO_STATUS_EXITING)) {
            return true;
        }
    }
    return false;
}

GeofenceZone_t* GeofenceManager::getMostCriticalGeofence() {
    GeofenceZone_t* most_critical = nullptr;
    float min_distance = 999999.0;
    
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        if (geofences[i].active && 
            (geofences[i].status == GEO_STATUS_OUTSIDE || 
             geofences[i].status == GEO_STATUS_EXITING)) {
            
            float abs_distance = abs(geofences[i].distance_to_boundary);
            if (abs_distance < min_distance) {
                min_distance = abs_distance;
                most_critical = &geofences[i];
            }
        }
    }
    
    return most_critical;
}

uint8_t GeofenceManager::getViolatedGeofencesCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        if (geofences[i].active && 
            (geofences[i].status == GEO_STATUS_OUTSIDE || 
             geofences[i].status == GEO_STATUS_EXITING)) {
            count++;
        }
    }
    return count;
}

// ============================================================================
// CONSULTAS
// ============================================================================
GeofenceZone_t* GeofenceManager::getGeofence(uint8_t id) {
    if (id >= MAX_GEOFENCES || !geofences[id].active) {
        return nullptr;
    }
    return &geofences[id];
}

float GeofenceManager::getClosestBoundaryDistance() {
    float closest_distance = 999999.0;
    
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        if (geofences[i].active) {
            float abs_distance = abs(geofences[i].distance_to_boundary);
            if (abs_distance < closest_distance) {
                closest_distance = abs_distance;
            }
        }
    }
    
    return closest_distance == 999999.0 ? 0.0 : closest_distance;
}

GeofenceStatus GeofenceManager::getOverallStatus() {
    if (active_count == 0) return GEO_STATUS_UNKNOWN;
    
    bool any_outside = false;
    bool any_exiting = false;
    bool all_inside = true;
    
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        if (geofences[i].active) {
            switch (geofences[i].status) {
                case GEO_STATUS_OUTSIDE:
                    any_outside = true;
                    all_inside = false;
                    break;
                case GEO_STATUS_EXITING:
                    any_exiting = true;
                    all_inside = false;
                    break;
                case GEO_STATUS_INSIDE:
                case GEO_STATUS_ENTERING:
                    // Continuar
                    break;
                default:
                    all_inside = false;
                    break;
            }
        }
    }
    
    if (any_outside) return GEO_STATUS_OUTSIDE;
    if (any_exiting) return GEO_STATUS_EXITING;
    if (all_inside) return GEO_STATUS_INSIDE;
    
    return GEO_STATUS_UNKNOWN;
}

// ============================================================================
// CONFIGURACIÓN POR DEFECTO
// ============================================================================
void GeofenceManager::setupDefaultGeofence(double center_lat, double center_lng, float radius) {
    clearAllGeofences();
    addCircleGeofence("Default", center_lat, center_lng, radius);
    DEBUG_PRINTF("🎯 Geocerca por defecto configurada: %.6f,%.6f, R=%.1fm\n", 
                 center_lat, center_lng, radius);
}

// ============================================================================
// INFORMACIÓN Y DEBUGGING
// ============================================================================
void GeofenceManager::printGeofenceInfo(uint8_t id) {
    if (id >= MAX_GEOFENCES || !geofences[id].active) {
        DEBUG_PRINTF("❌ Geocerca ID=%d no válida\n", id);
        return;
    }
    
    GeofenceZone_t& geo = geofences[id];
    
    DEBUG_PRINTF("🎯 === GEOCERCA ID=%d ===\n", id);
    DEBUG_PRINTF("Nombre: %s\n", geo.name);
    DEBUG_PRINTF("Activa: %s\n", geo.active ? "Sí" : "No");
    DEBUG_PRINTF("Forma: %s\n", 
                 geo.shape == GEO_SHAPE_CIRCLE ? "Círculo" : 
                 geo.shape == GEO_SHAPE_RECTANGLE ? "Rectángulo" : "Desconocida");
    
    if (geo.shape == GEO_SHAPE_CIRCLE) {
        DEBUG_PRINTF("Centro: %.6f, %.6f\n", geo.center.lat, geo.center.lng);
        DEBUG_PRINTF("Radio: %.1f m\n", geo.radius);
    }
    
    DEBUG_PRINTF("Estado: %s\n", 
                 geo.status == GEO_STATUS_INSIDE ? "DENTRO" :
                 geo.status == GEO_STATUS_OUTSIDE ? "FUERA" :
                 geo.status == GEO_STATUS_ENTERING ? "ENTRANDO" :
                 geo.status == GEO_STATUS_EXITING ? "SALIENDO" : "DESCONOCIDO");
    
    DEBUG_PRINTF("Distancia al límite: %.1f m\n", geo.distance_to_boundary);
    DEBUG_PRINTF("Tiempo dentro: %lu s\n", geo.time_inside / 1000);
    DEBUG_PRINTF("Tiempo fuera: %lu s\n", geo.time_outside / 1000);
    DEBUG_PRINTLN(F("========================"));
}

void GeofenceManager::printAllGeofences() {
    DEBUG_PRINTF("🎯 === TODAS LAS GEOCERCAS (%d activas) ===\n", active_count);
    
    if (active_count == 0) {
        DEBUG_PRINTLN(F("No hay geocercas activas"));
        return;
    }
    
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        if (geofences[i].active) {
            printGeofenceInfo(i);
        }
    }
}

String GeofenceManager::getStatusString() {
    if (active_count == 0) {
        return "Geofence: NONE";
    }
    
    GeofenceStatus overall = getOverallStatus();
    uint8_t violated = getViolatedGeofencesCount();
    
    String status_text;
    switch (overall) {
        case GEO_STATUS_INSIDE: status_text = "INSIDE"; break;
        case GEO_STATUS_OUTSIDE: status_text = "OUTSIDE"; break;
        case GEO_STATUS_ENTERING: status_text = "ENTERING"; break;
        case GEO_STATUS_EXITING: status_text = "EXITING"; break;
        default: status_text = "UNKNOWN"; break;
    }
    
    return "Geo: " + status_text + " (" + String(active_count) + "/" + String(violated) + ")";
}

// ============================================================================
// TEST Y SIMULACIÓN
// ============================================================================
void GeofenceManager::runSelfTest() {
    DEBUG_PRINTLN(F("🧪 === SELF-TEST GEOFENCING ==="));
    
    // Limpiar y crear geocerca de test
    clearAllGeofences();
    uint8_t test_id = addCircleGeofence("Test", -33.4489, -70.6693, 50.0);
    
    if (test_id == 255) {
        DEBUG_PRINTLN(F("❌ Error creando geocerca de test"));
        return;
    }
    
    // Test de posiciones
    GeoPoint_t test_positions[] = {
        {-33.4489, -70.6693},    // Centro (dentro)
        {-33.4485, -70.6693},    // Norte (dentro)
        {-33.4495, -70.6693},    // Sur (fuera)
        {-33.4489, -70.6688},    // Este (fuera)
        {-33.4489, -70.6698}     // Oeste (fuera)
    };
    
    for (int i = 0; i < 5; i++) {
        updatePosition(test_positions[i].lat, test_positions[i].lng);
        checkAllGeofences();
        
        GeofenceZone_t* geo = getGeofence(test_id);
        if (geo) {
            DEBUG_PRINTF("Test %d: Pos(%.6f,%.6f) -> %s, dist=%.1fm\n", 
                         i + 1, test_positions[i].lat, test_positions[i].lng,
                         geo->status == GEO_STATUS_INSIDE ? "INSIDE" : "OUTSIDE",
                         geo->distance_to_boundary);
        }
        
        delay(100);
    }
    
    // Limpiar
    clearAllGeofences();
    DEBUG_PRINTLN(F("✅ Self-test geofencing completado"));
}

void GeofenceManager::simulateMovement(double start_lat, double start_lng, double end_lat, double end_lng, uint8_t steps) {
    DEBUG_PRINTF("🎬 Simulando movimiento: (%.6f,%.6f) -> (%.6f,%.6f) en %d pasos\n", 
                 start_lat, start_lng, end_lat, end_lng, steps);
    
    double lat_step = (end_lat - start_lat) / steps;
    double lng_step = (end_lng - start_lng) / steps;
    
    for (uint8_t i = 0; i <= steps; i++) {
        double current_lat = start_lat + (lat_step * i);
        double current_lng = start_lng + (lng_step * i);
        
        updatePosition(current_lat, current_lng);
        checkAllGeofences();
        
        DEBUG_PRINTF("Paso %d: (%.6f,%.6f), Status: %s\n", 
                     i, current_lat, current_lng, getStatusString().c_str());
        
        delay(500);
    }
    
    DEBUG_PRINTLN(F("🎬 Simulación completada"));
}