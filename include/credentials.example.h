/**
 * ============================================================================
 * CONFIGURACIÓN DE CREDENCIALES LORAWAN - EJEMPLO
 * ============================================================================
 * IMPORTANTE: 
 * 1. Copiar este archivo como "credentials.h"
 * 2. Reemplazar con tus propias credenciales de ChirpStack/TTN
 * 3. NO subir credentials.h a Git (está en .gitignore)
 * 
 * @file credentials.example.h
 * @version 3.0.0
 */

#ifndef CREDENTIALS_H
#define CREDENTIALS_H

// ============================================================================
// CREDENCIALES LORAWAN (OTAA)
// ============================================================================

// Device EUI - Identificador único del dispositivo (64 bits)
// Obtener de ChirpStack: Devices -> Your Device -> Device EUI
const uint8_t LORAWAN_DEV_EUI[8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Application EUI (JoinEUI) - Identificador de la aplicación (64 bits)
// Obtener de ChirpStack: Applications -> Your App -> Application EUI
const uint8_t LORAWAN_APP_EUI[8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Application Key - Clave de cifrado (128 bits)
// Obtener de ChirpStack: Devices -> Your Device -> Application Key
const uint8_t LORAWAN_APP_KEY[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// ============================================================================
// CREDENCIALES LORAWAN (ABP) - Opcional
// ============================================================================

// Device Address - Dirección del dispositivo (32 bits)
const uint8_t LORAWAN_DEV_ADDR[4] = {
    0x00, 0x00, 0x00, 0x00
};

// Network Session Key - Clave de sesión de red (128 bits)
const uint8_t LORAWAN_NWK_SKEY[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Application Session Key - Clave de sesión de aplicación (128 bits)
const uint8_t LORAWAN_APP_SKEY[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// ============================================================================
// CONFIGURACIÓN DE RED
// ============================================================================

// Método de activación: true = OTAA, false = ABP
#define USE_OTAA true

// Región LoRaWAN
// EU868 = 0, US915 = 1, AU915 = 2, AS923 = 3, IN865 = 4
#define LORAWAN_REGION_CODE 2  // AU915 para Chile

// Sub-banda para US915/AU915 (1-8)
// Chile típicamente usa sub-banda 1 o 2
#define LORAWAN_SUB_BAND 1

// ============================================================================
// CONFIGURACIÓN OPCIONAL
// ============================================================================

// Servidor ChirpStack (si usas servidor propio)
#define CHIRPSTACK_SERVER "lorawan.example.com"
#define CHIRPSTACK_PORT 8080

// API Keys (para integraciones)
#define API_KEY "your-api-key-here"
#define API_SECRET "your-api-secret-here"

#endif // CREDENTIALS_H
