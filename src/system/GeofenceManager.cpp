#include "GeofenceManager.h"
#include "../core/Logger.h"

// ============================================================================
// CONSTRUCTOR E INICIALIZACIÓN
// ============================================================================

GeofenceManager::GeofenceManager() :
    initialized(false),
    active(false),
    geofenceCount(0),
    violationsCount(0),
    lastViolationTime(0),
    minDistanceRecorded(999999.0f),
    lastInsideState(true),
    lastAlertLevel(AlertLevel::SAFE),
    geofenceCallback(nullptr),
    violationCallback(nullptr)
{
    // Inicializar todas las geocercas como inactivas
    for (uint8_t i = 0; i < MAX_GEOFENCES; i++) {
        geofenceActive[i] = false;
    }
    
    initializeThresholds();
}

Result GeofenceManager::init() {
    if (initialized) {
        return Result::SUCCESS;
    }
    
    LOG_I("📍 Inicializando Geofence Manager...");
    
    // ❌ NO cargar geocerca por defecto al inicio (SEGURIDAD)
    // Solo inicializar como vacío - sin geocerca activa
    primaryGeofence = Geofence();
    primaryGeofence.active = false;
    active = false;
    
    // Reset completo de estadísticas
    violationsCount = 0;
    lastViolationTime = 0;
    minDistanceRecorded = 999999.0f;
    lastInsideState = true;
    lastAlertLevel = AlertLevel::SAFE;
    geofenceCount = 0;
    
    initialized = true;
    LOG_I("✅ Geofence Manager inicializado SIN geocerca por defecto");
    LOG_I("🛡️ Esperando configuración de geocerca desde backend por seguridad");
    
    return Result::SUCCESS;
}

bool GeofenceManager::isInitialized() const {
    return initialized;
}

// ============================================================================
// GESTIÓN DE GEOCERCA PRINCIPAL
// ============================================================================

void GeofenceManager::setGeofence(double centerLat, double centerLng, float radius, const char* name, const char* groupId) {
    Geofence newGeofence(centerLat, centerLng, radius, name, groupId);
    setGeofence(newGeofence);
}

void GeofenceManager::setGeofence(const Geofence& geofence) {
    if (!isValidGeofence(geofence)) {
        LOG_E("📍 Geocerca inválida");
        return;
    }
    
    primaryGeofence = geofence;
    primaryGeofence.active = true;
    active = true;  // Activar automáticamente
    
    if (primaryGeofence.type == GeofenceType::CIRCLE) {
        LOG_I("📍 Geocerca CÍRCULO configurada: %s - %.6f,%.6f R=%.1fm [Grupo: %s]", 
              primaryGeofence.name, primaryGeofence.centerLat, 
              primaryGeofence.centerLng, primaryGeofence.radius, primaryGeofence.groupId);
    } else {
        LOG_I("📍 Geocerca POLÍGONO configurada: %s - %d puntos [Grupo: %s]", 
              primaryGeofence.name, primaryGeofence.pointCount, primaryGeofence.groupId);
    }
    
    // Reset estadísticas
    minDistanceRecorded = 999999.0f;
    lastInsideState = true;
    lastAlertLevel = AlertLevel::SAFE;
    
    // ❌ NO GUARDAR EN MEMORIA PERSISTENTE POR SEGURIDAD
    LOG_I("🛡️ Geocerca configurada solo en memoria (no persistente por seguridad)");
}

void GeofenceManager::setPolygonGeofence(const GeoPoint* points, uint8_t numPoints, const char* name, const char* groupId) {
    if (!isValidPolygonGeofence(points, numPoints)) {
        LOG_E("📍 Polígono inválido: %d puntos", numPoints);
        return;
    }
    
    Geofence polygonGeofence(points, numPoints, name, groupId);
    setGeofence(polygonGeofence);
}

Geofence GeofenceManager::getGeofence() const {
    return primaryGeofence;
}

// ============================================================================
// CONTROL DE ACTIVACIÓN
// ============================================================================

void GeofenceManager::activate(bool enabled) {
    active = enabled;
    primaryGeofence.active = enabled;
    
    LOG_I("📍 Geocerca %s", enabled ? "activada" : "desactivada");
    
    if (enabled) {
        // Reset estadísticas al activar
        lastInsideState = true;
        lastAlertLevel = AlertLevel::SAFE;
    }
}

bool GeofenceManager::isActive() const {
    return active && primaryGeofence.active;
}

// ============================================================================
// VERIFICACIÓN DE POSICIÓN - CÍRCULOS Y POLÍGONOS
// ============================================================================

bool GeofenceManager::isInsideGeofence(const Position& position) const {
    if (!isValidPosition(position)) return false;
    return isInsideGeofence(position.latitude, position.longitude);
}

bool GeofenceManager::isInsideGeofence(double lat, double lng) const {
    if (!isActive()) return true; // Si no está activa, considerar siempre "inside"
    
    if (primaryGeofence.type == GeofenceType::CIRCLE) {
        return isPositionInsideCircle(primaryGeofence, lat, lng);
    } else {
        return isPositionInsidePolygon(primaryGeofence, lat, lng);
    }
}

float GeofenceManager::getDistance(const Position& position) const {
    if (!isValidPosition(position)) return 999999.0f;
    return getDistance(position.latitude, position.longitude);
}

float GeofenceManager::getDistance(double lat, double lng) const {
    if (!isActive()) return 0.0f;
    
    if (primaryGeofence.type == GeofenceType::CIRCLE) {
        return distanceToCircleBoundary(primaryGeofence, lat, lng);
    } else {
        return distanceToPolygonBoundaryInternal(primaryGeofence, lat, lng);
    }
}

// ============================================================================
// INFORMACIÓN DE LA GEOCERCA
// ============================================================================

double GeofenceManager::getCenterLat() const {
    return primaryGeofence.centerLat;
}

double GeofenceManager::getCenterLng() const {
    return primaryGeofence.centerLng;
}

float GeofenceManager::getRadius() const {
    return primaryGeofence.radius;
}

const char* GeofenceManager::getName() const {
    return primaryGeofence.name;
}

const char* GeofenceManager::getGroupId() const {
    return primaryGeofence.groupId;
}

GeofenceType GeofenceManager::getType() const {
    return primaryGeofence.type;
}

// ============================================================================
// INFORMACIÓN ESPECÍFICA PARA POLÍGONOS
// ============================================================================

uint8_t GeofenceManager::getPolygonPointCount() const {
    return (primaryGeofence.type == GeofenceType::POLYGON) ? primaryGeofence.pointCount : 0;
}

GeoPoint GeofenceManager::getPolygonPoint(uint8_t index) const {
    if (primaryGeofence.type == GeofenceType::POLYGON && index < primaryGeofence.pointCount) {
        return primaryGeofence.points[index];
    }
    return GeoPoint();
}

bool GeofenceManager::hasValidPolygon() const {
    return (primaryGeofence.type == GeofenceType::POLYGON) && 
           (primaryGeofence.pointCount >= 3) && 
           primaryGeofence.active;
}

// ============================================================================
// ANÁLISIS Y ESTADÍSTICAS
// ============================================================================

AlertLevel GeofenceManager::calculateAlertLevel(const Position& position) const {
    if (!isValidPosition(position)) return AlertLevel::SAFE;
    float distance = getDistance(position);
    return calculateAlertLevel(distance);
}

AlertLevel GeofenceManager::calculateAlertLevel(float distance) const {
    // Si está dentro de la geocerca, zona segura
    if (distance <= 0) {
        return AlertLevel::SAFE;
    }
    
    // Buscar el nivel de alerta correspondiente
    for (uint8_t i = 0; i < THRESHOLD_COUNT; i++) {
        if (distance <= thresholds[i].distance) {
            return thresholds[i].level;
        }
    }
    
    // Si excede todos los umbrales, emergencia
    return AlertLevel::EMERGENCY;
}

uint32_t GeofenceManager::getViolationsCount() const {
    return violationsCount;
}

uint32_t GeofenceManager::getLastViolationTime() const {
    return lastViolationTime;
}

float GeofenceManager::getMinDistanceRecorded() const {
    return minDistanceRecorded;
}

// ============================================================================
// MÚLTIPLES GEOCERCAS (Para expansión futura)
// ============================================================================

Result GeofenceManager::addGeofence(const Geofence& geofence, uint8_t* index) {
    if (geofenceCount >= MAX_GEOFENCES) {
        return Result::ERROR_NO_MEMORY;
    }
    
    if (!isValidGeofence(geofence)) {
        return Result::ERROR_INVALID_PARAM;
    }
    
    uint8_t newIndex = geofenceCount;
    geofences[newIndex] = geofence;
    geofenceActive[newIndex] = true;
    geofenceCount++;
    
    if (index) {
        *index = newIndex;
    }
    
    LOG_I("📍 Geocerca %d añadida: %s", newIndex, geofence.name);
    return Result::SUCCESS;
}

Result GeofenceManager::removeGeofence(uint8_t index) {
    if (index >= geofenceCount) {
        return Result::ERROR_INVALID_PARAM;
    }
    
    // Mover las geocercas posteriores hacia adelante
    for (uint8_t i = index; i < geofenceCount - 1; i++) {
        geofences[i] = geofences[i + 1];
        geofenceActive[i] = geofenceActive[i + 1];
    }
    
    geofenceCount--;
    LOG_I("📍 Geocerca %d eliminada", index);
    return Result::SUCCESS;
}

Result GeofenceManager::updateGeofence(uint8_t index, const Geofence& geofence) {
    if (index >= geofenceCount) {
        return Result::ERROR_INVALID_PARAM;
    }
    
    if (!isValidGeofence(geofence)) {
        return Result::ERROR_INVALID_PARAM;
    }
    
    geofences[index] = geofence;
    LOG_I("📍 Geocerca %d actualizada: %s", index, geofence.name);
    return Result::SUCCESS;
}

uint8_t GeofenceManager::getGeofenceCount() const {
    return geofenceCount;
}

Geofence GeofenceManager::getGeofence(uint8_t index) const {
    if (index < geofenceCount) {
        return geofences[index];
    }
    return Geofence(); // Geocerca vacía si el índice es inválido
}

// ============================================================================
// VERIFICACIÓN CON MÚLTIPLES GEOCERCAS
// ============================================================================

bool GeofenceManager::isInsideAnyGeofence(const Position& position) const {
    if (!isValidPosition(position)) return false;
    
    // Verificar geocerca principal
    if (isInsideGeofence(position)) {
        return true;
    }
    
    // Verificar geocercas adicionales
    for (uint8_t i = 0; i < geofenceCount; i++) {
        if (geofenceActive[i]) {
            if (geofences[i].type == GeofenceType::CIRCLE) {
                if (isPositionInsideCircle(geofences[i], position.latitude, position.longitude)) {
                    return true;
                }
            } else {
                if (isPositionInsidePolygon(geofences[i], position.latitude, position.longitude)) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

float GeofenceManager::getMinDistance(const Position& position) const {
    if (!isValidPosition(position)) return 999999.0f;
    
    float minDist = getDistance(position);
    
    // Verificar distancia a todas las geocercas
    for (uint8_t i = 0; i < geofenceCount; i++) {
        if (geofenceActive[i]) {
            float dist;
            if (geofences[i].type == GeofenceType::CIRCLE) {
                dist = distanceToCircleBoundary(geofences[i], position.latitude, position.longitude);
            } else {
                dist = distanceToPolygonBoundaryInternal(geofences[i], position.latitude, position.longitude);
            }
            
            if (dist < minDist) {
                minDist = dist;
            }
        }
    }
    
    return minDist;
}

AlertLevel GeofenceManager::getHighestAlertLevel(const Position& position) const {
    AlertLevel highest = calculateAlertLevel(position);
    
    // Verificar nivel de alerta de todas las geocercas
    for (uint8_t i = 0; i < geofenceCount; i++) {
        if (geofenceActive[i]) {
            float dist;
            if (geofences[i].type == GeofenceType::CIRCLE) {
                dist = distanceToCircleBoundary(geofences[i], position.latitude, position.longitude);
            } else {
                dist = distanceToPolygonBoundaryInternal(geofences[i], position.latitude, position.longitude);
            }
            
            AlertLevel level = calculateAlertLevel(dist);
            if (level > highest) {
                highest = level;
            }
        }
    }
    
    return highest;
}

// ============================================================================
// CALLBACKS Y UPDATE
// ============================================================================

void GeofenceManager::setGeofenceCallback(GeofenceCallback callback) {
    geofenceCallback = callback;
}

void GeofenceManager::setViolationCallback(ViolationCallback callback) {
    violationCallback = callback;
}

void GeofenceManager::update(const Position& currentPosition) {
    if (!initialized || !isActive() || !isValidPosition(currentPosition)) {
        return;
    }
    
    // Actualizar estadísticas
    updateStatistics(currentPosition);
    
    // Verificar violaciones
    checkViolations(currentPosition);
    
    // Ejecutar callbacks
    triggerCallbacks(currentPosition);
    
    // Guardar posición para próxima iteración
    lastPosition = currentPosition;
}

// ============================================================================
// GESTIÓN DE MEMORIA (Sin persistencia por seguridad)
// ============================================================================

void GeofenceManager::clearCurrentGeofence() {
    primaryGeofence = Geofence();
    primaryGeofence.active = false;
    active = false;
    
    // Reset estadísticas
    violationsCount = 0;
    lastViolationTime = 0;
    minDistanceRecorded = 999999.0f;
    lastInsideState = true;
    lastAlertLevel = AlertLevel::SAFE;
    
    LOG_I("🗑️ Geocerca eliminada de memoria");
}

void GeofenceManager::resetToDefaults() {
    // ❌ NO configurar geocerca por defecto (SEGURIDAD)
    primaryGeofence = Geofence();
    primaryGeofence.active = false;
    active = false;
    
    // Reset completo de estadísticas
    violationsCount = 0;
    lastViolationTime = 0;
    minDistanceRecorded = 999999.0f;
    lastInsideState = true;
    lastAlertLevel = AlertLevel::SAFE;
    geofenceCount = 0;
    
    LOG_I("🔄 Sistema reseteado - SIN geocerca por defecto por seguridad");
}

// ============================================================================
// UTILIDADES DE CÁLCULO ESTÁTICAS
// ============================================================================

float GeofenceManager::calculateDistance(double lat1, double lng1, double lat2, double lng2) {
    // Fórmula de Haversine para cálculo preciso de distancia
    double dLat = (lat2 - lat1) * DEG_TO_RAD;
    double dLng = (lng2 - lng1) * DEG_TO_RAD;
    
    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1 * DEG_TO_RAD) * cos(lat2 * DEG_TO_RAD) *
               sin(dLng / 2) * sin(dLng / 2);
    
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return EARTH_RADIUS_M * c;
}

bool GeofenceManager::isValidCoordinate(double lat, double lng) {
    return lat >= -90.0 && lat <= 90.0 && lng >= -180.0 && lng <= 180.0;
}

// ============================================================================
// ALGORITMOS PARA POLÍGONOS - IMPLEMENTACIÓN RAY-CASTING
// ============================================================================

bool GeofenceManager::isPointInPolygon(double lat, double lng, const GeoPoint* points, uint8_t numPoints) {
    if (numPoints < 3) return false;
    
    bool inside = false;
    
    // Algoritmo Ray-casting
    for (uint8_t i = 0, j = numPoints - 1; i < numPoints; j = i++) {
        if (((points[i].lat > lat) != (points[j].lat > lat)) &&
            (lng < (points[j].lng - points[i].lng) * (lat - points[i].lat) / (points[j].lat - points[i].lat) + points[i].lng)) {
            inside = !inside;
        }
    }
    
    return inside;
}

float GeofenceManager::distanceToPolygonBoundary(double lat, double lng, const GeoPoint* points, uint8_t numPoints) {
    if (numPoints < 3) return 999999.0f;
    
    float minDistance = 999999.0f;
    
    // Calcular distancia a cada segmento del polígono
    for (uint8_t i = 0; i < numPoints; i++) {
        uint8_t j = (i + 1) % numPoints;
        float segmentDistance = distanceToLineSegment(lat, lng, points[i], points[j]);
        
        if (segmentDistance < minDistance) {
            minDistance = segmentDistance;
        }
    }
    
    // Si está dentro del polígono, la distancia es negativa
    if (isPointInPolygon(lat, lng, points, numPoints)) {
        minDistance = -minDistance;
    }
    
    return minDistance;
}

float GeofenceManager::distanceToLineSegment(double lat, double lng, const GeoPoint& p1, const GeoPoint& p2) {
    // Convertir a metros usando proyección plana local (válida para distancias cortas)
    double lat0 = (p1.lat + p2.lat) / 2.0;
    double x = (lng - p1.lng) * cos(lat0 * DEG_TO_RAD) * 111320.0;
    double y = (lat - p1.lat) * 110540.0;
    double x1 = (p1.lng - p1.lng) * cos(lat0 * DEG_TO_RAD) * 111320.0; // = 0
    double y1 = (p1.lat - p1.lat) * 110540.0; // = 0
    double x2 = (p2.lng - p1.lng) * cos(lat0 * DEG_TO_RAD) * 111320.0;
    double y2 = (p2.lat - p1.lat) * 110540.0;
    
    double A = x - x1;
    double B = y - y1;
    double C = x2 - x1;
    double D = y2 - y1;
    
    double dot = A * C + B * D;
    double lenSq = C * C + D * D;
    
    if (lenSq < 1e-6) {
        // Segmento degenerado
        return sqrt(A * A + B * B);
    }
    
    double param = dot / lenSq;
    
    double xx, yy;
    
    if (param < 0) {
        xx = x1;
        yy = y1;
    } else if (param > 1) {
        xx = x2;
        yy = y2;
    } else {
        xx = x1 + param * C;
        yy = y1 + param * D;
    }
    
    double dx = x - xx;
    double dy = y - yy;
    
    return sqrt(dx * dx + dy * dy);
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

void GeofenceManager::initializeThresholds() {
    // Configurar umbrales por defecto basados en constantes
    thresholds[0].distance = CAUTION_DISTANCE;
    thresholds[0].level = AlertLevel::CAUTION;
    
    thresholds[1].distance = WARNING_DISTANCE;
    thresholds[1].level = AlertLevel::WARNING;
    
    thresholds[2].distance = DANGER_DISTANCE;
    thresholds[2].level = AlertLevel::DANGER;
    
    thresholds[3].distance = EMERGENCY_DISTANCE;
    thresholds[3].level = AlertLevel::EMERGENCY;
}

void GeofenceManager::updateStatistics(const Position& position) {
    float distance = getDistance(position);
    
    // Actualizar distancia mínima registrada
    if (distance >= 0 && distance < minDistanceRecorded) {
        minDistanceRecorded = distance;
    }
}

void GeofenceManager::checkViolations(const Position& position) {
    bool currentlyInside = isInsideGeofence(position);
    AlertLevel currentLevel = calculateAlertLevel(position);
    
    // Detectar nueva violación (salida de geocerca)
    if (lastInsideState && !currentlyInside) {
        violationsCount++;
        lastViolationTime = millis();
        
        float distance = getDistance(position);
        LOG_W("📍 Violación de geocerca #%d - Distancia: %.1fm [%s]", 
              violationsCount, distance, geofenceTypeToString(primaryGeofence.type));
        
        if (violationCallback) {
            violationCallback(primaryGeofence, distance, currentLevel);
        }
    }
    
    lastInsideState = currentlyInside;
    lastAlertLevel = currentLevel;
}

void GeofenceManager::triggerCallbacks(const Position& position) {
    if (geofenceCallback) {
        bool inside = isInsideGeofence(position);
        geofenceCallback(primaryGeofence, position, inside);
    }
}

bool GeofenceManager::isValidGeofence(const Geofence& geofence) const {
    if (!isValidCoordinate(geofence.centerLat, geofence.centerLng)) {
        return false;
    }
    
    if (geofence.type == GeofenceType::CIRCLE) {
        return geofence.radius >= MIN_GEOFENCE_RADIUS && geofence.radius <= MAX_GEOFENCE_RADIUS;
    } else {
        return isValidPolygonGeofence(geofence.points, geofence.pointCount);
    }
}

bool GeofenceManager::isValidPolygonGeofence(const GeoPoint* points, uint8_t numPoints) const {
    if (numPoints < 3 || numPoints > 10) { // MAX_POLYGON_POINTS = 10
        return false;
    }
    
    // Verificar que todos los puntos sean coordenadas válidas
    for (uint8_t i = 0; i < numPoints; i++) {
        if (!isValidCoordinate(points[i].lat, points[i].lng)) {
            return false;
        }
    }
    
    // Verificar que el polígono no sea degenerado (área > 0)
    // Cálculo simplificado del área usando fórmula del zapato
    double area = 0.0;
    for (uint8_t i = 0; i < numPoints; i++) {
        uint8_t j = (i + 1) % numPoints;
        area += (points[j].lng - points[i].lng) * (points[j].lat + points[i].lat);
    }
    area = abs(area) / 2.0;
    
    // Convertir aproximadamente a metros cuadrados
    area *= 111320.0 * 110540.0; // Aproximación muy burda
    
    return area > MIN_POLYGON_AREA;
}

// ============================================================================
// UTILIDADES INTERNAS - CÍRCULOS
// ============================================================================

float GeofenceManager::distanceToCircleBoundary(const Geofence& geofence, double lat, double lng) const {
    // Calcular distancia al centro
    float distanceToCenter = calculateDistance(geofence.centerLat, geofence.centerLng, lat, lng);
    
    // Distancia al borde (negativa si está dentro)
    return distanceToCenter - geofence.radius;
}

bool GeofenceManager::isPositionInsideCircle(const Geofence& geofence, double lat, double lng) const {
    if (!geofence.active) return true;
    
    float distance = calculateDistance(geofence.centerLat, geofence.centerLng, lat, lng);
    return distance <= geofence.radius;
}

// ============================================================================
// UTILIDADES INTERNAS - POLÍGONOS
// ============================================================================

float GeofenceManager::distanceToPolygonBoundaryInternal(const Geofence& geofence, double lat, double lng) const {
    if (geofence.type != GeofenceType::POLYGON || geofence.pointCount < 3) {
        return 999999.0f;
    }
    
    return distanceToPolygonBoundary(lat, lng, geofence.points, geofence.pointCount);
}

bool GeofenceManager::isPositionInsidePolygon(const Geofence& geofence, double lat, double lng) const {
    if (!geofence.active || geofence.type != GeofenceType::POLYGON) {
        return true;
    }
    
    return isPointInPolygon(lat, lng, geofence.points, geofence.pointCount);
}
