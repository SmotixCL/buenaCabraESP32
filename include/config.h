/**
 * ============================================================================
 * COLLAR GEOFENCING V3.0 - CONFIGURACIÓN GLOBAL
 * ============================================================================
 * 
 * Archivo de configuración centralizado para el collar LoRaWAN
 * Hardware: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)
 * 
 * @file config.h
 * @version 3.0
 * @date 2025
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// CONFIGURACIÓN DE HARDWARE - HELTEC V3
// ============================================================================

// *** PINS LORA SX1262 ***
#define LORA_NSS    8     // Chip Select
#define LORA_RST    12    // Reset
#define LORA_DIO1   14    // DIO1/IRQ
#define LORA_BUSY   13    // BUSY
#define LORA_SCK    9     // SPI Clock
#define LORA_MISO   11    // SPI MISO
#define LORA_MOSI   10    // SPI MOSI

// *** PINS OLED V3 ***
#define OLED_SDA    17    // I2C Data
#define OLED_SCL    18    // I2C Clock  
#define OLED_RST    21    // Reset
#define OLED_ADDRESS 0x3C // Dirección I2C

// *** PINS CONTROL ***
#define LED_PIN     35    // LED integrado
#define BUZZER_PIN  7     // Buzzer pasivo
#define VBAT_PIN    1     // ADC batería (GPIO1)

// *** PINS GPS ***
#define GPS_RX_PIN  3     // GPS Serial RX
#define GPS_TX_PIN  4     // GPS Serial TX

// *** PINS EXPANSIÓN ***
#define EXP_PIN_1   32    // Pin expansión 1
#define EXP_PIN_2   33    // Pin expansión 2

// ============================================================================
// CONFIGURACIÓN DE RADIO
// ============================================================================

// *** CONFIGURACIÓN LORA BÁSICA ***
#define LORA_FREQUENCY      915.0     // MHz (AU915)
#define LORA_BANDWIDTH      125.0     // kHz
#define LORA_SPREADING_FACTOR 9       // SF9
#define LORA_CODING_RATE    8         // 4/8
#define LORA_OUTPUT_POWER   20        // dBm
#define LORA_CURRENT_LIMIT  120       // mA

// *** CONFIGURACIÓN LORAWAN ***
#define LORAWAN_REGION      915       // AU915
#define LORAWAN_ADR         true      // Adaptive Data Rate
#define LORAWAN_CONFIRMED   false     // Mensajes confirmados

// ============================================================================
// CONFIGURACIÓN DE GEOFENCING
// ============================================================================

// *** DISTANCIAS DE ALERTA ***
#define GEOFENCE_SAFE_DISTANCE      15.0f    // >15m = zona segura
#define GEOFENCE_CAUTION_DISTANCE   10.0f    // 10-15m = precaución
#define GEOFENCE_WARNING_DISTANCE    5.0f    // 5-10m = advertencia
#define GEOFENCE_DANGER_DISTANCE     0.0f    // <5m = peligro

// *** CONFIGURACIÓN DEFAULT ***
#define GEOFENCE_DEFAULT_RADIUS     100.0f   // 100 metros
#define GEOFENCE_DEFAULT_LAT        -33.4489 // Santiago, Chile
#define GEOFENCE_DEFAULT_LNG        -70.6693

// ============================================================================
// CONFIGURACIÓN DE BUZZER
// ============================================================================

// *** FRECUENCIAS OPTIMIZADAS ***
#define BUZZER_FREQ_LOW         2000    // Hz - Baja penetración
#define BUZZER_FREQ_MID         2730    // Hz - Frecuencia óptima
#define BUZZER_FREQ_HIGH        3400    // Hz - Alta atención
#define BUZZER_FREQ_EMERGENCY   4000    // Hz - Emergencia

// *** NOTAS MUSICALES ***
#define NOTE_C4  262
#define NOTE_E4  330
#define NOTE_G4  392
#define NOTE_C5  523
#define NOTE_E5  659
#define NOTE_G5  784

// *** CONFIGURACIÓN PWM ***
#define BUZZER_PWM_TIMER        LEDC_TIMER_0
#define BUZZER_PWM_CHANNEL      LEDC_CHANNEL_0
#define BUZZER_PWM_RESOLUTION   LEDC_TIMER_10_BIT  // 1024 niveles
#define BUZZER_PWM_SPEED_MODE   LEDC_LOW_SPEED_MODE

// ============================================================================
// CONFIGURACIÓN DE TIEMPOS
// ============================================================================

// *** INTERVALOS DE TRANSMISIÓN ***
#define TX_INTERVAL_NORMAL      120000    // 2 minutos
#define TX_INTERVAL_ALERT       30000     // 30 segundos

// *** INTERVALOS DE VERIFICACIÓN ***
#define GEOFENCE_CHECK_INTERVAL 5000      // 5 segundos
#define OLED_UPDATE_INTERVAL    3000      // 3 segundos
#define BATTERY_CHECK_INTERVAL  60000     // 1 minuto
#define HEARTBEAT_INTERVAL      30000     // 30 segundos

// *** TIEMPOS DE ALERTA ***
#define ALERT_ESCALATION_TIME   60000     // 60 segundos para escalar
#define ALERT_PATTERN_BASE      8000      // Patrón base en ms

// ============================================================================
// CONFIGURACIÓN DE BATERÍA
// ============================================================================

// *** VOLTAJES ***
#define BATTERY_MIN_VOLTAGE     3.0f      // Voltaje mínimo
#define BATTERY_MAX_VOLTAGE     4.2f      // Voltaje máximo
#define BATTERY_LOW_VOLTAGE     3.3f      // Voltaje bajo
#define BATTERY_CRITICAL_VOLTAGE 3.1f     // Voltaje crítico

// *** ADC ***
#define BATTERY_ADC_RESOLUTION  4095.0f   // 12-bit ADC
#define BATTERY_VOLTAGE_DIVIDER 2.0f      // Divisor de voltaje 2:1
#define BATTERY_REFERENCE_VOLTAGE 3.3f    // Voltaje de referencia

// ============================================================================
// CONFIGURACIÓN DEL SISTEMA
// ============================================================================

// *** VERSIÓN ***
#define FIRMWARE_VERSION        "3.0.0"
#define HARDWARE_VERSION        "V3"
#define BUILD_DATE              __DATE__
#define BUILD_TIME              __TIME__

// *** CONFIGURACIÓN SERIAL ***
#define SERIAL_BAUD_RATE        115200
#define SERIAL_TIMEOUT          5000      // 5 segundos

// *** CONFIGURACIÓN GPS ***
#define GPS_BAUD_RATE           9600
#define GPS_TIMEOUT             60000     // 60 segundos
#define GPS_MIN_SATELLITES      4         // Mínimo satélites

// *** DEEP SLEEP ***
#define DEEP_SLEEP_DURATION     300000000 // 5 minutos en microsegundos
#define WAKEUP_ON_MOTION        true      // Despertar por movimiento

// ============================================================================
// MACROS DE DEBUG
// ============================================================================

// *** NIVELES DE DEBUG ***
#define DEBUG_LEVEL_NONE        0
#define DEBUG_LEVEL_ERROR       1
#define DEBUG_LEVEL_WARN        2
#define DEBUG_LEVEL_INFO        3
#define DEBUG_LEVEL_DEBUG       4

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_LEVEL_INFO
#endif

// *** MACROS DEBUG ***
#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
#define DEBUG_ERROR(x) Serial.print("[ERROR] "); Serial.println(x)
#else
#define DEBUG_ERROR(x)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_WARN
#define DEBUG_WARN(x) Serial.print("[WARN] "); Serial.println(x)
#else
#define DEBUG_WARN(x)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_INFO
#define DEBUG_INFO(x) Serial.print("[INFO] "); Serial.println(x)
#else
#define DEBUG_INFO(x)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_DEBUG
#define DEBUG_DEBUG(x) Serial.print("[DEBUG] "); Serial.println(x)
#else
#define DEBUG_DEBUG(x)
#endif

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

// *** ESTADO DEL SISTEMA ***
enum SystemState {
    SYSTEM_INIT,
    SYSTEM_RUNNING,
    SYSTEM_SLEEP,
    SYSTEM_ERROR,
    SYSTEM_LOW_POWER
};

// *** NIVELES DE ALERTA ***
enum AlertLevel {
    ALERT_SAFE = 0,
    ALERT_CAUTION = 1,
    ALERT_WARNING = 2,
    ALERT_DANGER = 3,
    ALERT_EMERGENCY = 4
};

// *** ESTRUCTURA DE POSICIÓN ***
struct Position {
    double latitude;
    double longitude;
    float altitude;
    bool valid;
    uint32_t timestamp;
    uint8_t satellites;
    float hdop;
};

// *** ESTRUCTURA DE GEOCERCA ***
struct Geofence {
    double center_lat;
    double center_lng;
    float radius;
    bool active;
    char name[32];
};

// *** ESTADO DE ALERTA (RTC) ***
struct AlertState {
    AlertLevel level;
    uint32_t start_time;
    uint32_t level_time;
    bool escalation_enabled;
    uint32_t pattern_count;
    float last_distance;
};

// *** ESTADO DE BATERÍA ***
struct BatteryStatus {
    float voltage;
    uint8_t percentage;
    bool low_power_mode;
    bool charging;
};

// *** ESTADO DEL HARDWARE ***
struct HardwareStatus {
    bool radio_initialized;
    bool oled_initialized;
    bool buzzer_initialized;
    bool gps_initialized;
    SystemState system_state;
};

#endif // CONFIG_H