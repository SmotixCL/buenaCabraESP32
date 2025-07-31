/*
 * CONFIGURACIÓN RADIOLIB PARA HELTEC WIFI LORA 32 V3
 * Reemplaza completamente la configuración LMIC (incompatible con SX1262)
 */

#ifndef RADIOLIB_PROJECT_CONFIG_H
#define RADIOLIB_PROJECT_CONFIG_H

// ============================================================================
// CONFIGURACIÓN ESPECÍFICA PARA SX1262 + ESP32-S3
// ============================================================================

// Habilitar modo de compatibilidad total con SX1262
#define RADIOLIB_GODMODE                1

// Configuración de debug para desarrollo
#define RADIOLIB_DEBUG                  1
#define RADIOLIB_VERBOSE               1

// ============================================================================
// CONFIGURACIÓN LORAWAN AU915 PARA CHILE
// ============================================================================

// Región y configuración de frecuencia
#define LORAWAN_REGION_AU915           1
#define LORAWAN_DEFAULT_SUB_BAND       2    // Sub-banda 2 (canales 8-15)

// Configuración de canales AU915
#define AU915_UPLINK_CH_0_7            1    // Canales 0-7 (916.8-918.2 MHz)
#define AU915_UPLINK_CH_8_15           1    // Canales 8-15 (919.0-920.4 MHz) 
#define AU915_UPLINK_CH_64             1    // Canal 64 (922.1 MHz)
#define AU915_DOWNLINK_CH_0_7          1    // Canales downlink

// Parámetros de red AU915
#define AU915_FREQUENCY_BASE           915000000   // 915 MHz
#define AU915_FREQUENCY_STEP           200000      // 200 kHz step
#define AU915_MAX_CHANNELS             72          // 64 uplink + 8 downlink

// ============================================================================
// CONFIGURACIÓN DE POTENCIA Y MODULACIÓN
// ============================================================================

// Potencia de transmisión (máximo permitido en Chile)
#define LORAWAN_DEFAULT_TX_POWER       20     // +20 dBm
#define LORAWAN_MAX_TX_POWER           20     // Límite legal Chile

// Configuración de spreading factor por defecto
#define LORAWAN_DEFAULT_SF             9      // SF9 para balance alcance/velocidad
#define LORAWAN_MIN_SF                 7      // SF7 mínimo
#define LORAWAN_MAX_SF                 12     // SF12 máximo

// Bandwidth por defecto
#define LORAWAN_DEFAULT_BW             125.0  // 125 kHz

// ============================================================================
// CONFIGURACIÓN DE TIMING Y PROTOCOLOS
// ============================================================================

// Versión del protocolo LoRaWAN
#define LORAWAN_VERSION                LORAWAN_VERSION_1_0_3

// Configuración de ADR (Adaptive Data Rate)
#define LORAWAN_ADR_ENABLED            1      // Habilitar ADR
#define LORAWAN_ADR_ACK_LIMIT          64     // Límite para solicitar ACK
#define LORAWAN_ADR_ACK_DELAY          32     // Delay para ACK

// Duty cycle (no aplica estrictamente en AU915 pero buena práctica)
#define LORAWAN_DUTY_CYCLE_ENABLED     0      // AU915 no tiene duty cycle estricto
#define LORAWAN_MAX_DUTY_CYCLE         100    // 100% permitido en AU915

// Configuración de retry
#define LORAWAN_DEFAULT_RETRIES        3      // 3 reintentos por defecto
#define LORAWAN_MAX_RETRIES            8      // Máximo 8 reintentos

// ============================================================================
// CONFIGURACIÓN DE MEMORIA Y PERFORMANCE
// ============================================================================

// Optimizaciones de memoria para ESP32-S3
#define RADIOLIB_STATIC_ONLY           0      // Permitir allocación dinámica
#define RADIOLIB_LOW_LEVEL             0      // Usar API de alto nivel

// Buffer sizes
#define LORAWAN_MAX_PAYLOAD_LENGTH     242    // Máximo payload LoRaWAN
#define LORAWAN_DEFAULT_RX_BUFFER      256    // Buffer de recepción
#define LORAWAN_DEFAULT_TX_BUFFER      256    // Buffer de transmisión

// ============================================================================
// CONFIGURACIÓN ESPECÍFICA PARA HELTEC V3
// ============================================================================

// Pins del hardware (ya definidos en main.cpp pero referencia aquí)
#define RADIOLIB_PIN_NSS               8      // Chip select
#define RADIOLIB_PIN_DIO1              14     // DIO1 interrupt
#define RADIOLIB_PIN_NRST              12     // Reset
#define RADIOLIB_PIN_BUSY              13     // Busy

// Configuración SPI
#define RADIOLIB_SPI_FREQUENCY         1000000  // 1 MHz para estabilidad
#define RADIOLIB_SPI_MODE              SPI_MODE0

// ============================================================================
// CONFIGURACIÓN DE JOIN Y KEYS
// ============================================================================

// Configuración de join
#define LORAWAN_JOIN_EUI_SIZE          8      // 8 bytes para Join EUI
#define LORAWAN_DEV_EUI_SIZE           8      // 8 bytes para Device EUI
#define LORAWAN_APP_KEY_SIZE           16     // 16 bytes para App Key
#define LORAWAN_NWK_KEY_SIZE           16     // 16 bytes para Network Key

// Timeouts para join
#define LORAWAN_JOIN_TIMEOUT_MS        30000  // 30 segundos timeout
#define LORAWAN_JOIN_RETRIES           5      // 5 reintentos de join
#define LORAWAN_JOIN_RETRY_DELAY_MS    60000  // 1 minuto entre reintentos

// ============================================================================
// CONFIGURACIÓN DE CLASE Y MODOS
// ============================================================================

// Clase LoRaWAN (A, B, C)
#define LORAWAN_CLASS                  LORAWAN_CLASS_A

// Modo de activación
#define LORAWAN_ACTIVATION_MODE        LORAWAN_ACTIVATION_OTAA  // Over-The-Air

// Confirmación de mensajes
#define LORAWAN_DEFAULT_CONFIRMED      0      // Sin confirmación por defecto
#define LORAWAN_CONFIRMED_RETRIES      3      // 3 reintentos para confirmed

// ============================================================================
// CONFIGURACIÓN DE SEGURIDAD
// ============================================================================

// Configuración de encriptación
#define LORAWAN_ENCRYPTION_ENABLED     1      // Siempre encriptar
#define LORAWAN_MIC_ENABLED            1      // Verificación de integridad

// Frame counter
#define LORAWAN_FCNT_UP_INITIAL        0      // Contador inicial uplink
#define LORAWAN_FCNT_DOWN_INITIAL      0      // Contador inicial downlink
#define LORAWAN_FCNT_MAX               0xFFFFFFFF  // Máximo contador

// ============================================================================
// CONFIGURACIÓN DE DOWNLINK
// ============================================================================

// Ventanas de recepción
#define LORAWAN_RX1_DELAY_MS           1000   // 1 segundo delay RX1
#define LORAWAN_RX2_DELAY_MS           2000   // 2 segundos delay RX2

// Configuración RX2
#define LORAWAN_RX2_FREQUENCY          923300000  // 923.3 MHz para AU915
#define LORAWAN_RX2_SF                 12         // SF12 para RX2

// ============================================================================
// CONFIGURACIÓN DE DEBUG Y LOGGING
// ============================================================================

#ifdef DEBUG_MODE
    #define RADIOLIB_DEBUG_BASIC       1
    #define RADIOLIB_DEBUG_PROTOCOL    1
    #define RADIOLIB_DEBUG_VERBOSE     1
    #define LORAWAN_DEBUG_ENABLED      1
#else
    #define RADIOLIB_DEBUG_BASIC       0
    #define RADIOLIB_DEBUG_PROTOCOL    0
    #define RADIOLIB_DEBUG_VERBOSE     0
    #define LORAWAN_DEBUG_ENABLED      0
#endif

// ============================================================================
// CONFIGURACIÓN DE COMPATIBILIDAD
// ============================================================================

// Compatibilidad con diferentes versiones de RadioLib
#define RADIOLIB_VERSION_CHECK         1
#define RADIOLIB_MIN_VERSION_MAJOR     7
#define RADIOLIB_MIN_VERSION_MINOR     1

// Compatibilidad con ESP32-S3
#define RADIOLIB_ESP32_S3_SUPPORT      1
#define RADIOLIB_FREERTOS_SUPPORT      1

// ============================================================================
// MACROS DE VERIFICACIÓN
// ============================================================================

// Verificación de configuración válida
#if !defined(LORAWAN_REGION_AU915)
    #error "Debe definirse LORAWAN_REGION_AU915 para uso en Chile"
#endif

#if LORAWAN_DEFAULT_TX_POWER > 20
    #error "Potencia máxima en Chile es +20 dBm"
#endif

#if LORAWAN_DEFAULT_SUB_BAND < 1 || LORAWAN_DEFAULT_SUB_BAND > 8
    #error "Sub-banda AU915 debe estar entre 1-8"
#endif

// ============================================================================
// CONFIGURACIONES EXPERIMENTALES (COMENTADAS POR DEFECTO)
// ============================================================================

// Descomenta para funcionalidades experimentales
// #define RADIOLIB_EXPERIMENTAL_FEATURES    1
// #define LORAWAN_EXPERIMENTAL_CLASS_B      1
// #define LORAWAN_EXPERIMENTAL_CLASS_C      1

// ============================================================================
// NOTAS DE CONFIGURACIÓN
// ============================================================================

/*
IMPORTANTE:
- Esta configuración está optimizada para Heltec WiFi LoRa 32 V3
- SX1262 requiere RadioLib, NO funciona con LMIC
- AU915 sub-banda 2 es compatible con The Things Network en Chile
- Potencia máxima +20 dBm es legal en Chile
- ADR está habilitado para optimización automática

TROUBLESHOOTING:
- Si join falla: verificar claves OTAA y conectividad del gateway
- Si transmisión falla: verificar sub-banda y frecuencias
- Si alcance es bajo: verificar antena y potencia de transmisión
- Si consumo es alto: deshabilitar debug y ajustar timing

PERSONALIZACIÓN:
- Cambiar LORAWAN_DEFAULT_SUB_BAND según tu proveedor
- Ajustar LORAWAN_DEFAULT_SF según balance alcance/velocidad
- Modificar timeouts según condiciones de red
*/

#endif // RADIOLIB_PROJECT_CONFIG_H