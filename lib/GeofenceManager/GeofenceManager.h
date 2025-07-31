/**
 * ============================================================================
 * COLLAR GEOFENCING - MÓDULO GEOFENCE
 * ============================================================================
 * 
 * Sistema de gestión de geocercas con alertas progresivas
 * 
 * @file GeofenceManager.h
 * @version 3.0
 */

#ifndef GEOFENCE_MANAGER_H
#define GEOFENCE_MANAGER_H

#include <Arduino.h>
#include "config.h"

class GeofenceManager {
private:
    static Geofence active_geofence;
    static AlertState alert_state;
    static bool initialized;
    static uint32_t last_check_time;
    static float last_distance;
    
    // Funciones internas
    static float calculateDistance(double lat1, double lng1, double lat2, double lng2);
    static AlertLevel calculateAlertLevel(float distance_to_limit);
    static void escalateAlert();

public:
    // *** INICIALIZACIÓN ***
    static bool init();
    static bool isInitialized();
    static void deinit();
    
    // *** GESTIÓN DE GEOCERCAS ***
    static bool setGeofence(double center_lat, double center_lng, float radius, const String& name = "");
    static bool setGeofence(const Geofence& geofence);
    static Geofence getActiveGeofence();
    static bool isGeofenceActive();
    static void enableGeofence();
    static void disableGeofence();
    
    // *** VERIFICACIÓN DE POSICIÓN ***
    static float checkPosition(const Position& position);
    static float getDistanceToLimit(const Position& position);
    static bool isInsideGeofence(const Position& position);
    static AlertLevel getCurrentAlertLevel();
    
    // *** SISTEMA DE ALERTAS ***
    static void startAlert(const Position& position);
    static void stopAlert();
    static void updateAlert(const Position& position);
    static AlertState getAlertState();
    static void resetAlertState();
    
    // *** CONFIGURACIÓN ***
    static void setAlertDistances(float safe, float caution, float warning, float danger);
    static void enableEscalation(bool enable);
    static void setEscalationTime(uint32_t time_ms);
    
    // *** UTILIDADES ***
    static float getBearing(const Position& from, const Position& to);
    static Position getClosestPointOnBoundary(const Position& position);
    static String getAlertLevelName(AlertLevel level);
    static String getDirectionToCenter(const Position& position);
    
    // *** INFORMACIÓN ***
    static void printStatus();
    static void printGeofenceInfo();
    static void printAlertInfo();
    
    // *** TESTING ***
    static void testWithSimulatedPositions();
    static void testAlertEscalation();
};

#endif // GEOFENCE_MANAGER_H