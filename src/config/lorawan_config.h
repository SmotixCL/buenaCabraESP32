/**
 * ============================================================================
 * COLLAR BUENACABRA V3.0 - CONFIGURACIÓN LORAWAN
 * ============================================================================
 * 
 * @file lorawan_config.h
 * @version 3.0.0
 */

#ifndef LORAWAN_CONFIG_H
#define LORAWAN_CONFIG_H

// ============================================================================
// CONFIGURACIÓN DE REGIÓN
// ============================================================================
#define LORAWAN_REGION 915          // AU915 para Chile
#define LORAWAN_FREQUENCY 915.0      // MHz
#define LORAWAN_BANDWIDTH 125.0      // kHz
#define LORAWAN_SF 9                 // Spreading Factor
#define LORAWAN_CODING_RATE 8        // 4/8
#define LORAWAN_OUTPUT_POWER 20      // dBm
#define LORAWAN_PREAMBLE_LENGTH 8

// ============================================================================
// PUERTOS LORAWAN
// ============================================================================
#define LORAWAN_PORT_GPS 2
#define LORAWAN_PORT_CONFIG 10
#define LORAWAN_PORT_ALERT 20
#define LORAWAN_PORT_STATUS 30

// ============================================================================
// INTERVALOS DE TRANSMISIÓN
// ============================================================================
#define TX_INTERVAL_NORMAL 60000      // 1 minuto normal
#define TX_INTERVAL_ALERT 30000       // 30 segundos en alerta
#define TX_INTERVAL_EMERGENCY 15000   // 15 segundos en emergencia

// ============================================================================
// CONFIGURACIÓN DE JOIN
// ============================================================================
#define JOIN_RETRY_INTERVAL 30000     // 30 segundos entre intentos
#define JOIN_MAX_RETRIES 10            // Máximo de reintentos

// ============================================================================
// ESTRUCTURA DE ACTUALIZACIÓN DE GEOCERCA
// ============================================================================
struct GeofenceUpdate {
    uint8_t type;           // 0=círculo, 1=polígono
    char name[32];          // Nombre de la geocerca
    char groupId[16];       // ID del grupo
    double centerLat;
    double centerLng;
    float radius;
    uint8_t pointCount;     // Número de puntos del polígono
    struct {
        double lat;
        double lng;
    } points[10];           // Array de puntos (máximo 10)
};

#endif // LORAWAN_CONFIG_H
