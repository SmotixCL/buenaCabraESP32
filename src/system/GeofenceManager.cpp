#include "GeofenceManager.h"
#include <Preferences.h>

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
    
    // NO cargar geocerca por defecto al inicio
    // Solo inicializar como vacío
    primaryGeofence = Geofence();
    primaryGeofence.active = false;
    active = false;
    
    // Intentar cargar configuración guardada
    if (loadConfiguration() == Result::SUCCESS) {
        LOG_I("📍 Geocerca cargada desde memoria");
    } else {
        LOG_I("📍 Sin geocerca previa, esperando configuración");
        // NO resetear a valores por defecto
    }
    
    initialized = true;
    LOG_I("✅ Geofence Manager inicializado");
    
    return Result::SUCCESS;
}

bool GeofenceManager::isInitialized() const {
    return initialized;
}

// ============================================================================
// GESTIÓN DE GEOCERCA PRINCIPAL
// ============================================================================

void GeofenceManager::setGeofence(double centerLat, double centerLng, float radius, const char* name) {
    Geofence newGeofence(centerLat, centerLng, radius, name);
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
    
    LOG_I("📍 Geocerca configurada: %s - %.6f,%.6f R=%.1fm", 
          primaryGeofence.name, primaryGeofence.centerLat, 
          primaryGeofence.centerLng, primaryGeofence.radius);
    
    // Reset estadísticas
    minDistanceRecorded = 999999.0f;
    lastInsideState = true;
    lastAlertLevel = AlertLevel::SAFE;
    
    // GUARDAR CONFIGURACIÓN AUTOMÁTICAMENTE
    if (saveConfiguration() == Result::SUCCESS) {
        LOG_I("💾 Geocerca guardada en memoria persistente");
    } else {
        LOG_W("⚠️ No se pudo guardar la geocerca en memoria");
    }
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
// VERIFICACIÓN DE POSICIÓN
// ============================================================================

bool GeofenceManager::isInsideGeofence(const Position& position) const {
    if (!isValidPosition(position)) return false;
    return isInsideGeofence(position.latitude, position.longitude);
}

bool GeofenceManager::isInsideGeofence(double lat, double lng) const {
    if (!isActive()) return true; // Si no está activa, considerar siempre "inside"
    
    return isPositionInside(primaryGeofence, lat, lng);
}

float GeofenceManager::getDistance(const Position& position) const {
    if (!isValidPosition(position)) return 999999.0f;
    return getDistance(position.latitude, position.longitude);
}

float GeofenceManager::getDistance(double lat, double lng) const {
    if (!isActive()) return 0.0f;
    
    return distanceToGeofenceBoundary(primaryGeofence, lat, lng);
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
// MÚLTIPLES GEOCERCAS
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
        if (geofenceActive[i] && isPositionInside(geofences[i], position.latitude, position.longitude)) {
            return true;
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
            float dist = distanceToGeofenceBoundary(geofences[i], position.latitude, position.longitude);
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
            float dist = distanceToGeofenceBoundary(geofences[i], position.latitude, position.longitude);
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
// PERSISTENCIA
// ============================================================================

Result GeofenceManager::saveConfiguration() {
    Preferences prefs;
    
    // Intentar abrir namespace en modo escritura
    bool opened = prefs.begin("geofence", false);
    if (!opened) {
        LOG_E("📍 Error al abrir namespace 'geofence' para escritura");
        return Result::ERROR_HARDWARE;
    }
    
    // Guardar geocerca principal
    prefs.putDouble("lat", primaryGeofence.centerLat);
    prefs.putDouble("lng", primaryGeofence.centerLng);
    prefs.putFloat("radius", primaryGeofence.radius);
    prefs.putBool("active", primaryGeofence.active);
    prefs.putString("name", primaryGeofence.name);
    
    // Guardar estadísticas
    prefs.putUInt("violations", violationsCount);
    prefs.putFloat("minDist", minDistanceRecorded);
    
    prefs.end();
    LOG_I("📍 Configuración guardada");
    return Result::SUCCESS;
}

Result GeofenceManager::loadConfiguration() {
    // WORKAROUND: Crear namespace si no existe
    Preferences prefs;
    
    // Primero intentar crear/abrir en modo escritura para asegurar que existe
    if (prefs.begin("geofence", false)) {
        prefs.end();
    }
    
    // Ahora abrir en modo lectura
    bool opened = prefs.begin("geofence", true);
    if (!opened) {
        LOG_W("📍 No se pudo abrir namespace 'geofence' para lectura");
        return Result::ERROR_HARDWARE;
    }
    
    // Verificar si hay configuración guardada
    if (!prefs.isKey("lat")) {
        prefs.end();
        LOG_D("📍 No hay geocerca guardada en memoria");
        return Result::ERROR_INVALID_PARAM;
    }
    
    // Cargar geocerca principal
    primaryGeofence.centerLat = prefs.getDouble("lat", -33.4489);
    primaryGeofence.centerLng = prefs.getDouble("lng", -70.6693);
    primaryGeofence.radius = prefs.getFloat("radius", DEFAULT_GEOFENCE_RADIUS);
    primaryGeofence.active = prefs.getBool("active", true);
    strcpy(primaryGeofence.name, prefs.getString("name", "Principal").c_str());
    
    // Cargar estadísticas
    violationsCount = prefs.getUInt("violations", 0);
    minDistanceRecorded = prefs.getFloat("minDist", 999999.0f);
    
    prefs.end();
    LOG_I("📍 Configuración cargada");
    return Result::SUCCESS;
}

void GeofenceManager::resetToDefaults() {
    // Configurar geocerca por defecto (Santiago, Chile)
    primaryGeofence = Geofence(-33.4489, -70.6693, DEFAULT_GEOFENCE_RADIUS, "Santiago");
    primaryGeofence.active = true;
    active = true;
    
    // Reset estadísticas
    violationsCount = 0;
    lastViolationTime = 0;
    minDistanceRecorded = 999999.0f;
    lastInsideState = true;
    lastAlertLevel = AlertLevel::SAFE;
    geofenceCount = 0;
    
    LOG_I("📍 Configuración reseteada a valores por defecto");
}

// ============================================================================
// UTILIDADES DE CÁLCULO
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
// MÉTODOS PRIVADOS
// ============================================================================

void GeofenceManager::initializeThresholds() {
    // Configurar umbrales por defecto basados en constantes
    thresholds[0] = {CAUTION_DISTANCE, AlertLevel::CAUTION};   // 10m
    thresholds[1] = {WARNING_DISTANCE, AlertLevel::WARNING};   // 5m
    thresholds[2] = {DANGER_DISTANCE, AlertLevel::DANGER};     // 2m
    thresholds[3] = {EMERGENCY_DISTANCE, AlertLevel::EMERGENCY}; // 0m
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
        LOG_W("📍 Violación de geocerca #%d - Distancia: %.1fm", violationsCount, distance);
        
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
    return isValidCoordinate(geofence.centerLat, geofence.centerLng) &&
           geofence.radius >= MIN_GEOFENCE_RADIUS &&
           geofence.radius <= MAX_GEOFENCE_RADIUS;
}

float GeofenceManager::distanceToGeofenceBoundary(const Geofence& geofence, double lat, double lng) const {
    // Calcular distancia al centro
    float distanceToCenter = calculateDistance(geofence.centerLat, geofence.centerLng, lat, lng);
    
    // Distancia al borde (negativa si está dentro)
    return distanceToCenter - geofence.radius;
}

bool GeofenceManager::isPositionInside(const Geofence& geofence, double lat, double lng) const {
    if (!geofence.active) return true;
    
    float distance = calculateDistance(geofence.centerLat, geofence.centerLng, lat, lng);
    return distance <= geofence.radius;
}
