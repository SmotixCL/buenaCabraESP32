#pragma once

/*
 * ============================================================================
 * CONFIGURACIÓN LORAWAN - COLLAR GEOFENCING
 * ============================================================================
 * 
 * IMPORTANTE: Cambiar estos valores por los reales de su aplicación
 * No usar estos valores en producción - son solo para ejemplo
 */

// ============================================================================
// CONFIGURACIÓN DE REGIÓN
// ============================================================================
#define LORAWAN_REGION_AU915        // Australia 915MHz (compatible con Chile)
#define LORAWAN_SUBBAND     1       // Sub-banda 1 (canales 8-15 + 65)

// ============================================================================
// CONFIGURACIÓN OTAA (RECOMENDADO)
// ============================================================================
// ============================================================================
// CONFIGURACIÓN OTAA (RECOMENDADO PARA DEBUGGING)
// ============================================================================
// Device EUI (8 bytes) - LITTLE ENDIAN PARA LORAWAN (ORDEN INVERTIDO)
const uint8_t LORAWAN_DEV_EUI[8] = {
    0x58, 0xEC, 0x3C, 0x43, 0xCA, 0x48, 0x00, 0x00  // ✅ CORREGIDO: little endian
};

// Application EUI (8 bytes) - LITTLE ENDIAN para LoRaWAN  
const uint8_t LORAWAN_APP_EUI[8] = {
    0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x00, 0x00, 0x01  // ✅ Orden correcto little endian
};

// Application Key (16 bytes) - CLAVE REAL DEL USUARIO
const uint8_t LORAWAN_APP_KEY[16] = {
    0x12, 0x8A, 0x9F, 0x0C, 0x8B, 0x8E, 0xFB, 0x6D,
    0xCD, 0x33, 0xC2, 0x37, 0x06, 0x27, 0x2E, 0x75  // ✅ Clave real configurada
};
// ============================================================================
// CONFIGURACIÓN ABP (USADA ACTUALMENTE)
// ============================================================================
// Device Address (4 bytes) - CLAVES REALES DE CHIRPSTACK
const uint8_t LORAWAN_DEV_ADDR[4] = {
    0x00, 0xEE, 0x93, 0x7A  // ✅ Claves reales del usuario
};

// Network Session Key (16 bytes) - CLAVES REALES DE CHIRPSTACK
const uint8_t LORAWAN_NWK_SKEY[16] = {
    0x33, 0x07, 0x5B, 0x9E, 0x6F, 0x6A, 0x87, 0x37, 
    0x40, 0x60, 0xD8, 0x98, 0xDD, 0x2B, 0xAC, 0xD8  // ✅ Claves reales del usuario
};

// Application Session Key (16 bytes) - CLAVES REALES DE CHIRPSTACK
const uint8_t LORAWAN_APP_SKEY[16] = {
    0x73, 0x3C, 0x3C, 0x0A, 0xF9, 0x83, 0xD5, 0x30, 
    0x20, 0xAF, 0xE7, 0xBA, 0xF0, 0x43, 0xE6, 0xE1  // ✅ Claves reales del usuario
};

// ============================================================================
// CONFIGURACIÓN DE RED
// ============================================================================
#define LORAWAN_USE_OTAA            true    // ✅ CAMBIAR A OTAA
#define LORAWAN_ADR_ENABLED         true    // Adaptive Data Rate
#define LORAWAN_CONFIRMED_UPLINKS   false   // Confirmación de uplinks
#define LORAWAN_DEFAULT_DATARATE    0       // DR0 (SF12, más alcance)
#define LORAWAN_DEFAULT_TX_POWER    20      // 20 dBm (máximo permitido)

// ============================================================================
// PUERTOS DE APLICACIÓN
// ============================================================================
#define LORAWAN_PORT_GPS            1       // Datos de posición GPS
#define LORAWAN_PORT_BATTERY        2       // Estado de batería
#define LORAWAN_PORT_SYSTEM         3       // Estado del sistema
#define LORAWAN_PORT_ALERTS         4       // Alertas y geocercas
#define LORAWAN_PORT_CONFIG         10      // Comandos de configuración

// ============================================================================
// CONFIGURACIÓN DE FRECUENCIAS (AU915)
// ============================================================================
// Sub-banda 1: 916.8, 917.0, 917.2, 917.4, 917.6, 917.8, 918.0, 918.2 MHz
// Downlink: 923.3 MHz

// ============================================================================
// INSTRUCCIONES DE CONFIGURACIÓN
// ============================================================================
/*
 * CÓMO CONFIGURAR LAS CLAVES LORAWAN:
 * 
 * 1. REGISTRO EN SERVIDOR LORAWAN:
 *    - Crear cuenta en The Things Network (TTN) o ChirpStack
 *    - Crear nueva aplicación
 *    - Añadir dispositivo end-device
 * 
 * 2. OBTENER CLAVES:
 *    - DevEUI: Generar automáticamente o usar MAC del ESP32
 *    - AppEUI: Copiar de la aplicación
 *    - AppKey: Copiar de la configuración del dispositivo
 * 
 * 3. CONFIGURAR REGIÓN:
 *    - Seleccionar AU915 en el servidor
 *    - Habilitar sub-banda 1 (canales 8-15)
 * 
 * 4. REEMPLAZAR VALORES:
 *    - Copiar las claves del servidor a este archivo
 *    - Cambiar todos los 0x00 por los valores reales
 * 
 * EJEMPLO DE CONFIGURACIÓN REAL:
 * const uint8_t LORAWAN_DEV_EUI[8] = {
 *     0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x04, 0xA6, 0x2B
 * };
 * 
 * NOTA DE SEGURIDAD:
 * - No subir este archivo con claves reales a repositorios públicos
 * - Usar variables de entorno en producción
 * - Rotar claves periódicamente
 */

// ============================================================================
// UTILIDADES PARA DEBUGGING
// ============================================================================
inline void printLoRaWANConfig() {
    #if DEBUG_ENABLED
    DEBUG_PRINTLN("📡 Configuración LoRaWAN:");
    DEBUG_PRINTF("   Región: AU915 Sub-banda %d\n", LORAWAN_SUBBAND);
    DEBUG_PRINTF("   Modo: %s\n", LORAWAN_USE_OTAA ? "OTAA" : "ABP");
    DEBUG_PRINTF("   ADR: %s\n", LORAWAN_ADR_ENABLED ? "Habilitado" : "Deshabilitado");
    DEBUG_PRINTF("   Data Rate: %d\n", LORAWAN_DEFAULT_DATARATE);
    DEBUG_PRINTF("   TX Power: %d dBm\n", LORAWAN_DEFAULT_TX_POWER);
    
    // DevEUI
    DEBUG_PRINT("   DevEUI: ");
    for (int i = 0; i < 8; i++) {
        DEBUG_PRINTF("%02X", LORAWAN_DEV_EUI[i]);
        if (i < 7) DEBUG_PRINT(":");
    }
    DEBUG_PRINTLN();
    
    // AppEUI  
    DEBUG_PRINT("   AppEUI: ");
    for (int i = 0; i < 8; i++) {
        DEBUG_PRINTF("%02X", LORAWAN_APP_EUI[i]);
        if (i < 7) DEBUG_PRINT(":");
    }
    DEBUG_PRINTLN();
    #endif
}

// Verificar si las claves están configuradas (no todos ceros)
inline bool isLoRaWANConfigured() {
    // Verificar DevEUI
    for (int i = 0; i < 8; i++) {
        if (LORAWAN_DEV_EUI[i] != 0x00) return true;
    }
    return false;
}
