#pragma once

/*
 * ============================================================================
 * CONSTANTES DEL SISTEMA - COLLAR GEOFENCING
 * ============================================================================
 */

// ============================================================================
// CONFIGURACIÓN GENERAL DEL SISTEMA
// ============================================================================
#define FIRMWARE_VERSION    "3.0.0"
#define DEVICE_NAME         "CollarGeo"
#define MANUFACTURER        "BuenaCabra"

// ============================================================================
// CONFIGURACIÓN DE GEOCERCA
// ============================================================================
#define DEFAULT_GEOFENCE_RADIUS     50.0f   // Radio por defecto (metros)
#define MIN_GEOFENCE_RADIUS         10.0f   // Radio mínimo
#define MAX_GEOFENCE_RADIUS         1000.0f // Radio máximo

// Distancias para niveles de alerta
#define SAFE_DISTANCE               15.0f   // >15m = zona segura
#define CAUTION_DISTANCE            10.0f   // 10-15m = precaución
#define WARNING_DISTANCE            5.0f    // 5-10m = advertencia
#define DANGER_DISTANCE             2.0f    // 2-5m = peligro
#define EMERGENCY_DISTANCE          0.0f    // <2m = emergencia

// ============================================================================
// CONFIGURACIÓN DE AUDIO/BUZZER
// ============================================================================
// Frecuencias optimizadas para penetración sonora
#define FREQ_SAFE              0       // Sin sonido
#define FREQ_CAUTION           2000    // Baja - suave aviso
#define FREQ_WARNING           2730    // Media - atención
#define FREQ_DANGER            3400    // Alta - alerta
#define FREQ_EMERGENCY         4000    // Máxima - urgencia

// Duración de tonos (ms)
#define TONE_DURATION_SHORT    200
#define TONE_DURATION_MEDIUM   500
#define TONE_DURATION_LONG     1000

// Volumen PWM (0-100%)
#define VOLUME_LOW             50
#define VOLUME_MEDIUM          75
#define VOLUME_HIGH            90
#define VOLUME_MAX             100

// ============================================================================
// NOTAS MUSICALES (Hz)
// ============================================================================
#define NOTE_C4     262
#define NOTE_D4     294
#define NOTE_E4     330
#define NOTE_F4     349
#define NOTE_G4     392
#define NOTE_A4     440
#define NOTE_B4     494
#define NOTE_C5     523
#define NOTE_D5     587
#define NOTE_E5     659
#define NOTE_F5     698
#define NOTE_G5     784

// ============================================================================
// CONFIGURACIÓN DE COMUNICACIÓN
// ============================================================================
#define TX_INTERVAL_NORMAL      600000   // 10 minutos modo normal (ms)
#define TX_INTERVAL_ALERT       30000    // 30 segundos en alerta (ms)
#define TX_INTERVAL_EMERGENCY   10000    // 10 segundos emergencia (ms)

#define MAX_LORAWAN_PAYLOAD_SIZE    32       // Tamaño máximo payload LoRaWAN
#define MAX_RETRY_ATTEMPTS      3        // Intentos de reenvío

// Configuración Serial
#define SERIAL_BAUD             115200   // Velocidad serie
#define SERIAL_TIMEOUT          5000     // Timeout conexión serie (ms)

// ============================================================================
// CONFIGURACIÓN DE ENERGÍA
// ============================================================================
#define VBAT_DIVIDER           2.0f      // Divisor de voltaje ADC
#define VBAT_REFERENCE         3.3f      // Voltaje referencia ADC
#define VBAT_RESOLUTION        4095      // Resolución ADC 12-bit

#define BATTERY_FULL           4.2f      // Batería llena (V)
#define BATTERY_GOOD           3.8f      // Batería buena (V)
#define BATTERY_LOW            3.6f      // Batería baja (V)
#define BATTERY_CRITICAL       3.4f      // Batería crítica (V)
#define BATTERY_EMPTY          3.0f      // Batería vacía (V)

// ============================================================================
// INTERVALOS DE TIEMPO (ms)
// ============================================================================
#define INTERVAL_GEOFENCE_CHECK     2000    // Verificar geocerca cada 2s
#define INTERVAL_DISPLAY_UPDATE     3000    // Actualizar OLED cada 3s
#define INTERVAL_BATTERY_CHECK      60000   // Leer batería cada 1min
#define INTERVAL_HEARTBEAT          30000   // Heartbeat cada 30s
#define INTERVAL_GPS_READ           5000    // Leer GPS cada 5s

// ============================================================================
// CONFIGURACIÓN OLED
// ============================================================================
#define OLED_WIDTH              128
#define OLED_HEIGHT             64
#define OLED_TIMEOUT_SLEEP      300000   // Apagar OLED después de 5min

// ============================================================================
// CONFIGURACIÓN GPS
// ============================================================================
#define GPS_TIMEOUT             10000    // Timeout GPS (ms)
#define GPS_MIN_SATELLITES      4        // Mínimo satélites para fix válido
#define GPS_ACCURACY_THRESHOLD  5.0f     // Precisión mínima (metros)

// ============================================================================
// DEBUG Y LOGGING
// ============================================================================
#define DEBUG_ENABLED           1        // Habilitar debug serial
#define LOG_LEVEL_ERROR         1
#define LOG_LEVEL_WARN          2
#define LOG_LEVEL_INFO          3
#define LOG_LEVEL_DEBUG         4

#ifndef LOG_LEVEL
    #define LOG_LEVEL           LOG_LEVEL_INFO
#endif

// ============================================================================
// MACROS DE DEBUG
// ============================================================================
#if DEBUG_ENABLED
    #define DEBUG_PRINT(x)      Serial.print(x)
    #define DEBUG_PRINTLN(x)    Serial.println(x)
    #define DEBUG_PRINTF(...)   Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

// Macros de logging por nivel
#if LOG_LEVEL >= LOG_LEVEL_ERROR
    #define LOG_ERROR(...)      Serial.printf("[ERROR] " __VA_ARGS__)
#else
    #define LOG_ERROR(...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
    #define LOG_WARN(...)       Serial.printf("[WARN] " __VA_ARGS__)
#else
    #define LOG_WARN(...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
    #define LOG_INFO(...)       Serial.printf("[INFO] " __VA_ARGS__)
#else
    #define LOG_INFO(...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    #define LOG_DEBUG(...)      Serial.printf("[DEBUG] " __VA_ARGS__)
#else
    #define LOG_DEBUG(...)
#endif

// ============================================================================
// ESTADOS DEL SISTEMA
// ============================================================================
enum SystemState {
    STATE_INIT = 0,
    STATE_NORMAL,
    STATE_ALERT,
    STATE_EMERGENCY,
    STATE_SLEEP,
    STATE_CONFIG,
    STATE_ERROR
};

// ============================================================================
// CONFIGURACIÓN DEL WATCHDOG
// ============================================================================
#define WATCHDOG_TIMEOUT        30       // Segundos para watchdog
#define ENABLE_WATCHDOG         true

// ============================================================================
// VALIDACIONES (usando valores enteros para preprocessor)
// ============================================================================
#define DEFAULT_GEOFENCE_RADIUS_INT     50
#define MIN_GEOFENCE_RADIUS_INT         10

#if DEFAULT_GEOFENCE_RADIUS_INT < MIN_GEOFENCE_RADIUS_INT
    #error "Radio de geocerca por defecto menor al mínimo"
#endif

#if TX_INTERVAL_EMERGENCY > TX_INTERVAL_ALERT
    #error "Intervalo de emergencia debe ser menor al de alerta"
#endif
