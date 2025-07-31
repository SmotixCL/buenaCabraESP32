/*
 * CONSTANTES GLOBALES PARA COLLAR LORAWAN V3.0
 * Definiciones centralizadas para fácil mantenimiento
 */

#ifndef COLLAR_CONSTANTS_H
#define COLLAR_CONSTANTS_H

// ============================================================================
// INFORMACIÓN DEL FIRMWARE
// ============================================================================

#define FIRMWARE_VERSION        "3.0.0"
#define FIRMWARE_BUILD_DATE     __DATE__
#define FIRMWARE_BUILD_TIME     __TIME__
#define HARDWARE_VERSION        "Heltec WiFi LoRa 32 V3"
#define AUTHOR                  "Sistema Geofencing LoRaWAN"

// ============================================================================
// CONFIGURACIÓN DE HARDWARE ESPECÍFICA V3
// ============================================================================

// Verificación de board correcto
#ifndef HELTEC_WIFI_LORA_32_V3
    #error "Este firmware está diseñado específicamente para Heltec WiFi LoRa 32 V3"
#endif

// Pins específicos V3 (diferentes de V1/V2)
#define HELTEC_V3_LORA_NSS      8
#define HELTEC_V3_LORA_RST      12
#define HELTEC_V3_LORA_DIO1     14
#define HELTEC_V3_LORA_BUSY     13
#define HELTEC_V3_LORA_SCK      9
#define HELTEC_V3_LORA_MISO     11
#define HELTEC_V3_LORA_MOSI     10

#define HELTEC_V3_OLED_SDA      17
#define HELTEC_V3_OLED_SCL      18
#define HELTEC_V3_OLED_RST      21

#define HELTEC_V3_LED           35
#define HELTEC_V3_BUZZER        7
#define HELTEC_V3_VBAT          1

// ============================================================================
// CONFIGURACIÓN LORAWAN PARA CHILE
// ============================================================================

// Región y frecuencias
#define LORAWAN_REGION_NAME     "AU915"
#define LORAWAN_COUNTRY         "Chile"
#define LORAWAN_SUB_BAND        2
#define LORAWAN_CHANNELS_COUNT  8

// Frecuencias específicas sub-banda 2 (MHz)
#define FREQ_CH8                919.0
#define FREQ_CH9                919.2
#define FREQ_CH10               919.4
#define FREQ_CH11               919.6
#define FREQ_CH12               919.8
#define FREQ_CH13               920.0
#define FREQ_CH14               920.2
#define FREQ_CH15               920.4
#define FREQ_DOWNLINK           923.3

// Parámetros de red
#define MAX_TX_POWER_DBM        20      // Máximo legal en Chile
#define DEFAULT_SF              9       // Balance alcance/velocidad
#define DEFAULT_BW_KHZ          125     // Bandwidth estándar
#define DEFAULT_CR              5       // Coding rate 4/5

// ============================================================================
// SISTEMA DE ALERTAS - FRECUENCIAS MUSICALES
// ============================================================================

// Frecuencias base (Hz) - optimizadas para penetración
#define BASE_FREQ_LOW           1800    // Graves - mayor alcance
#define BASE_FREQ_MID           2730    // Media - máxima penetración
#define BASE_FREQ_HIGH          3400    // Aguda - máxima atención
#define BASE_FREQ_EMERGENCY     4000    // Emergencia - urgencia máxima

// Notas musicales para melodías (Hz)
#define MUSICAL_C4              262
#define MUSICAL_D4              294
#define MUSICAL_E4              330
#define MUSICAL_F4              349
#define MUSICAL_G4              392
#define MUSICAL_A4              440
#define MUSICAL_B4              494
#define MUSICAL_C5              523
#define MUSICAL_D5              587
#define MUSICAL_E5              659
#define MUSICAL_F5              698
#define MUSICAL_G5              784
#define MUSICAL_A5              880
#define MUSICAL_B5              988
#define MUSICAL_C6              1047

// ============================================================================
// SISTEMA DE GEOCERCA - PARÁMETROS GEOGRÁFICOS
// ============================================================================

// Constantes geográficas
#define EARTH_RADIUS_KM         6371.0
#define DEGREES_TO_RADIANS      (PI / 180.0)
#define METERS_PER_DEGREE_LAT   111000.0    // Aproximado
#define GPS_PRECISION_METERS    5.0         // Precisión esperada GPS

// Límites de geocerca
#define MIN_GEOFENCE_RADIUS     10.0        // 10 metros mínimo
#define MAX_GEOFENCE_RADIUS     5000.0      // 5 km máximo
#define DEFAULT_GEOFENCE_RADIUS 100.0       // 100 metros por defecto

// Coordenadas por defecto (Santiago, Chile)
#define DEFAULT_LATITUDE        -33.4489
#define DEFAULT_LONGITUDE       -70.6693
#define DEFAULT_ALTITUDE        500.0

// ============================================================================
// GESTIÓN DE ENERGÍA
// ============================================================================

// Voltajes de batería LiPo (V)
#define BATTERY_MAX_VOLTAGE     4.2         // Totalmente cargada
#define BATTERY_FULL_VOLTAGE    4.1         // Considerada llena
#define BATTERY_GOOD_VOLTAGE    3.8         // Buen estado
#define BATTERY_LOW_VOLTAGE     3.6         // Nivel bajo
#define BATTERY_CRITICAL_VOLTAGE 3.3        // Crítico
#define BATTERY_MIN_VOLTAGE     3.0         // Mínimo absoluto

// Consumos estimados (mA)
#define CURRENT_ACTIVE          120         // Transmitiendo
#define CURRENT_GPS_SEARCH      40          // GPS buscando
#define CURRENT_IDLE            15          // En espera
#define CURRENT_DEEP_SLEEP      0.005       // 5 µA en deep sleep

// Capacidades de batería típicas (mAh)
#define BATTERY_SMALL           1000        // Batería pequeña
#define BATTERY_STANDARD        2000        // Batería estándar
#define BATTERY_LARGE           3000        // Batería grande
#define BATTERY_EXTENDED        5000        // Batería extendida

// ============================================================================
// TIMING Y INTERVALOS
// ============================================================================

// Intervalos de transmisión (segundos)
#define TX_INTERVAL_FAST        60          // 1 minuto (testing)
#define TX_INTERVAL_NORMAL      600         // 10 minutos (normal)
#define TX_INTERVAL_SLOW        3600        // 1 hora (ahorro)
#define TX_INTERVAL_EMERGENCY   30          // 30 segundos (alerta)

// Timeouts (milisegundos)
#define GPS_TIMEOUT_MS          60000       // 1 minuto para fix GPS
#define LORAWAN_JOIN_TIMEOUT_MS 30000       // 30 segundos para join
#define LORAWAN_TX_TIMEOUT_MS   10000       // 10 segundos para TX
#define BUZZER_MAX_DURATION_MS  5000        // 5 segundos máximo continuo

// Intervalos de verificación (milisegundos)
#define GEOFENCE_CHECK_INTERVAL 5000        // 5 segundos
#define BATTERY_CHECK_INTERVAL  60000       // 1 minuto
#define HEARTBEAT_INTERVAL      30000       // 30 segundos
#define OLED_UPDATE_INTERVAL    5000        // 5 segundos

// ============================================================================
// CONFIGURACIÓN DE DATOS Y PROTOCOLOS
// ============================================================================

// Tamaños de payload
#define LORAWAN_PAYLOAD_SIZE    12          // 12 bytes optimizado
#define GPS_DATA_SIZE          16          // Datos GPS completos
#define SENSOR_DATA_SIZE       8           // Datos de sensores

// Offsets para encoding de coordenadas
#define LATITUDE_OFFSET        90.0         // +90° para convertir a positivo
#define LONGITUDE_OFFSET       180.0        // +180° para convertir a positivo
#define COORDINATE_MULTIPLIER  1000000      // 6 decimales de precisión

// Códigos de estado
#define STATUS_GPS_VALID       0x01         // Bit 0: GPS válido
#define STATUS_BATTERY_LOW     0x02         // Bit 1: Batería baja
#define STATUS_GEOFENCE_ALERT  0x04         // Bit 2: Alerta geocerca
#define STATUS_EMERGENCY       0x08         // Bit 3: Emergencia
#define STATUS_MOTION_DETECTED 0x10         // Bit 4: Movimiento detectado

// ============================================================================
// CÓDIGOS DE DOWNLINK
// ============================================================================

#define CMD_ALERT_MANUAL       0x01         // Activar alerta manual
#define CMD_GEOFENCE_UPDATE    0x02         // Actualizar geocerca
#define CMD_BUZZER_TEST        0x03         // Test sistema buzzer
#define CMD_CONFIG_UPDATE      0x04         // Actualizar configuración
#define CMD_RESET_DEVICE       0x05         // Reset del dispositivo
#define CMD_SLEEP_MODE         0x06         // Activar modo sleep
#define CMD_WAKE_UP            0x07         // Despertar dispositivo
#define CMD_STATUS_REQUEST     0x08         // Solicitar status completo

// ============================================================================
// CONFIGURACIÓN DE DEBUG Y LOGGING
// ============================================================================

// Niveles de debug
#define DEBUG_LEVEL_NONE       0
#define DEBUG_LEVEL_ERROR      1
#define DEBUG_LEVEL_WARNING    2
#define DEBUG_LEVEL_INFO       3
#define DEBUG_LEVEL_DEBUG      4
#define DEBUG_LEVEL_VERBOSE    5

// Prefijos de log
#define LOG_PREFIX_ERROR       "❌ ERROR: "
#define LOG_PREFIX_WARNING     "⚠️  WARN:  "
#define LOG_PREFIX_INFO        "ℹ️  INFO:  "
#define LOG_PREFIX_DEBUG       "🔧 DEBUG: "
#define LOG_PREFIX_VERBOSE     "📝 TRACE: "

// ============================================================================
// LÍMITES Y VALIDACIONES
// ============================================================================

// Límites de configuración
#define MAX_PACKET_COUNTER     65535       // uint16 máximo
#define MAX_ALERT_DURATION     300000      // 5 minutos máximo alerta continua
#define MAX_GPS_RETRIES        10          // Máximo reintentos GPS
#define MAX_LORAWAN_RETRIES    5           // Máximo reintentos LoRaWAN

// Validaciones de datos
#define MIN_VALID_LATITUDE     -90.0
#define MAX_VALID_LATITUDE     90.0
#define MIN_VALID_LONGITUDE    -180.0
#define MAX_VALID_LONGITUDE    180.0
#define MIN_VALID_ALTITUDE     -500.0      // Bajo nivel del mar
#define MAX_VALID_ALTITUDE     10000.0     // 10 km máximo

// ============================================================================
// CONFIGURACIÓN DE FUTURAS EXPANSIONES
// ============================================================================

// Pins reservados para expansiones futuras
#define EXPANSION_PIN_1        32
#define EXPANSION_PIN_2        33
#define EXPANSION_PIN_3        25
#define EXPANSION_PIN_4        26

// Protocolo de comunicación con módulos externos
#define EXT_PROTOCOL_UART      1
#define EXT_PROTOCOL_I2C       2
#define EXT_PROTOCOL_SPI       3
#define EXT_PROTOCOL_GPIO      4

// Configuración sistema de descarga (futuro)
#define HV_ENABLE_PIN          32
#define HV_CONTROL_PIN         33
#define HV_SAFETY_PIN          25
#define HV_MAX_VOLTAGE         12.0        // 12V máximo seguro
#define HV_MAX_CURRENT         0.5         // 500mA máximo
#define HV_PULSE_DURATION      1000        // 1 segundo máximo

// ============================================================================
// MACROS DE UTILIDAD
// ============================================================================

// Conversiones útiles
#define MS_TO_SECONDS(ms)      ((ms) / 1000)
#define SECONDS_TO_MS(s)       ((s) * 1000)
#define MINUTES_TO_MS(m)       ((m) * 60 * 1000)
#define HOURS_TO_MS(h)         ((h) * 60 * 60 * 1000)

// Macros de validación
#define IS_VALID_LAT(lat)      ((lat) >= MIN_VALID_LATITUDE && (lat) <= MAX_VALID_LATITUDE)
#define IS_VALID_LNG(lng)      ((lng) >= MIN_VALID_LONGITUDE && (lng) <= MAX_VALID_LONGITUDE)
#define IS_VALID_VOLTAGE(v)    ((v) >= BATTERY_MIN_VOLTAGE && (v) <= BATTERY_MAX_VOLTAGE)

// Macros de cálculo
#define VOLTAGE_TO_PERCENT(v)  (((v) - BATTERY_MIN_VOLTAGE) * 100.0 / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE))
#define PERCENT_TO_VOLTAGE(p)  (BATTERY_MIN_VOLTAGE + ((p) * (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) / 100.0))

// ============================================================================
// STRINGS DE CONFIGURACIÓN
// ============================================================================

// Mensajes de estado
#define MSG_SYSTEM_INIT        "🚀 Sistema inicializando..."
#define MSG_SYSTEM_READY       "✅ Sistema listo"
#define MSG_LORAWAN_JOINING    "🔗 Conectando a LoRaWAN..."
#define MSG_LORAWAN_JOINED     "✅ LoRaWAN conectado"
#define MSG_GPS_SEARCHING      "📡 Buscando GPS..."
#define MSG_GPS_FOUND          "📍 GPS localizado"
#define MSG_GEOFENCE_ALERT     "🚨 Alerta de geocerca"
#define MSG_BATTERY_LOW        "🔋 Batería baja"
#define MSG_ENTERING_SLEEP     "😴 Entrando en modo sleep"

// Identificadores de dispositivo
#define DEVICE_MANUFACTURER    "Heltec Automation"
#define DEVICE_MODEL           "WiFi LoRa 32 V3"
#define DEVICE_TYPE            "LoRaWAN Geofencing Collar"
#define DEVICE_CLASS           "Agricultural IoT"

#endif // COLLAR_CONSTANTS_H