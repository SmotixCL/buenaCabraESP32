/**
 * ============================================================================
 * COLLAR BUENACABRA V3.0 - CONSTANTES DEL SISTEMA
 * ============================================================================
 *
 * @file constants.h
 * @version 3.0.0
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

// ============================================================================
// INFORMACIÓN DEL FIRMWARE
// ============================================================================
#define FIRMWARE_VERSION "3.0.0"
#define MANUFACTURER "BuenaCabra"
#define DEVICE_NAME "Collar-GPS-LoRa"

// ============================================================================
// CONFIGURACIÓN DE COMUNICACIÓN
// ============================================================================
#define SERIAL_BAUD 115200
#define SERIAL_TIMEOUT 5000
#define I2C_FREQUENCY 400000
#define GPS_BAUD_RATE 9600

// ============================================================================
// INTERVALOS DE TIEMPO (ms)
// ============================================================================
#define GPS_UPDATE_INTERVAL 5000
#define BATTERY_CHECK_INTERVAL 60000
#define DISPLAY_UPDATE_INTERVAL 2000
#define LORA_TX_INTERVAL 60000
#define HEARTBEAT_INTERVAL 10000
#define SERIAL_STATUS_INTERVAL 30000
#define GEOFENCE_CHECK_INTERVAL 10000

// ============================================================================
// CONFIGURACIÓN DE GPS
// ============================================================================
#define GPS_FIX_TIMEOUT 180000
#define GPS_MIN_SATELLITES 4
#define GPS_HDOP_THRESHOLD 2.0

// ============================================================================
// CONFIGURACIÓN DE DISPLAY
// ============================================================================
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_CONTRAST 255

// ============================================================================
// CONFIGURACIÓN DE BUZZER
// ============================================================================
#define BUZZER_DEFAULT_FREQUENCY 1000
#define BUZZER_DEFAULT_DURATION 100
#define BUZZER_DEFAULT_VOLUME 128
#define BUZZER_PWM_CHANNEL 0
#define BUZZER_PWM_RESOLUTION 8

// ============================================================================
// CONFIGURACIÓN DE GEOCERCA
// ============================================================================
#define GEOFENCE_MAX_NAME_LENGTH 32
#define DISTANCE_SAFE 0
#define DISTANCE_CAUTION 50
#define DISTANCE_WARNING 150
#define DISTANCE_DANGER 300
#define DISTANCE_EMERGENCY 500

// Alias para compatibilidad
#define CAUTION_DISTANCE DISTANCE_CAUTION
#define WARNING_DISTANCE DISTANCE_WARNING
#define DANGER_DISTANCE DISTANCE_DANGER
#define EMERGENCY_DISTANCE DISTANCE_EMERGENCY

// Límites de geocerca
#define MIN_GEOFENCE_RADIUS 10.0f
#define MAX_GEOFENCE_RADIUS 10000.0f

// Límites de batería
#define BATTERY_LOW 3.3f
#define BATTERY_CRITICAL 3.1f

// ============================================================================
// CONFIGURACIÓN DE SISTEMA
// ============================================================================
#define WATCHDOG_TIMEOUT 30000
#define MIN_FREE_HEAP 10000
#define PREF_NAMESPACE "collar"

#endif // CONSTANTS_H
