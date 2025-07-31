/*
 * CONFIGURACIÓN DE CLAVES LORAWAN - V3.0
 * 
 * ⚠️  IMPORTANTE: CADA COLLAR DEBE TENER CLAVES ÚNICAS
 * 
 * Este archivo contiene las claves de seguridad LoRaWAN.
 * Debe ser configurado individualmente para cada collar.
 */

#ifndef LORAWAN_KEYS_H
#define LORAWAN_KEYS_H

// ============================================================================
// CONFIGURACIÓN DE ACTIVACIÓN
// ============================================================================

// Método de activación: OTAA (recomendado) o ABP
#define USE_OTAA                1       // 1 = OTAA, 0 = ABP

// ============================================================================
// CLAVES OTAA (OVER-THE-AIR ACTIVATION) - RECOMENDADO
// ============================================================================

#if USE_OTAA

// JoinEUI (anteriormente AppEUI) - 8 bytes
// CAMBIAR por valores únicos proporcionados por el proveedor LoRaWAN
static const uint8_t joinEUI[8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// DevEUI - 8 bytes - DEBE SER ÚNICO POR DISPOSITIVO
// Usar MAC address o generar aleatoriamente
// Formato: LSB primero (little endian)
static const uint8_t devEUI[8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// AppKey - 16 bytes - SECRETA, ÚNICA POR DISPOSITIVO
// NUNCA compartir esta clave públicamente
static const uint8_t appKey[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// NwkKey - 16 bytes - Para LoRaWAN 1.0.x es igual que AppKey
static const uint8_t nwkKey[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#endif

// ============================================================================
// CLAVES ABP (ACTIVATION BY PERSONALIZATION) - ALTERNATIVO
// ============================================================================

#if !USE_OTAA

// DevAddr - 4 bytes - Dirección del dispositivo
// Debe estar en el rango asignado por el proveedor
static const uint32_t devAddr = 0x00000000;

// NwkSKey - 16 bytes - Clave de red
static const uint8_t nwkSKey[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// AppSKey - 16 bytes - Clave de aplicación
static const uint8_t appSKey[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#endif

// ============================================================================
// CONFIGURACIÓN DE RED ESPECÍFICA PARA CHILE
// ============================================================================

// Región LoRaWAN
#define LORAWAN_REGION          AU915

// Sub-banda para Chile (The Things Network)
#define LORAWAN_SUB_BAND        2       // Sub-banda 2 (canales 8-15)

// Configuración de canales AU915 Sub-banda 2
// Frecuencias: 919.0, 919.2, 919.4, 919.6, 919.8, 920.0, 920.2, 920.4 MHz
// Downlink: 923.3 MHz

// ============================================================================
// EJEMPLOS DE CONFIGURACIÓN POR PROVEEDOR
// ============================================================================

/*
EJEMPLO - THE THINGS NETWORK (TTN):

joinEUI: Proporcionado por TTN (generalmente todos ceros para TTN v3)
devEUI:  Generar único, ej: basado en MAC del ESP32
appKey:  Generar en consola TTN, copiar aquí

Para generar DevEUI único basado en MAC:
1. Leer MAC del ESP32
2. Usar como base para DevEUI
3. Asegurar que sea único por collar

EJEMPLO - CHIRPSTACK:

Configurar en ChirpStack server:
1. Crear device profile para AU915
2. Crear application
3. Crear device con claves generadas
4. Copiar claves aquí
*/

// ============================================================================
// CONFIGURACIÓN DE TESTING
// ============================================================================

// Para testing en laboratorio - NO USAR EN PRODUCCIÓN
#ifdef TESTING_MODE

// Claves de prueba - CAMBIAR ANTES DE PRODUCCIÓN
#warning "¡USANDO CLAVES DE TESTING! Cambiar antes de producción"

#undef joinEUI
#undef devEUI  
#undef appKey
#undef nwkKey

static const uint8_t joinEUI[8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t devEUI[8] = {
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x01  // Testing DevEUI
};

static const uint8_t appKey[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

static const uint8_t nwkKey[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

#endif

// ============================================================================
// UTILIDADES PARA GENERACIÓN DE CLAVES
// ============================================================================

// Función para generar DevEUI basado en MAC ESP32
// Llamar durante setup() si DevEUI es todo ceros
void generateDevEUIFromMAC() {
    uint8_t baseMac[6];
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    
    // Solo generar si DevEUI actual es todo ceros
    bool allZeros = true;
    for (int i = 0; i < 8; i++) {
        if (devEUI[i] != 0) {
            allZeros = false;
            break;
        }
    }
    
    if (allZeros) {
        Serial.println("⚠️  DevEUI es todo ceros, generando desde MAC...");
        
        // Usar MAC como base para DevEUI único
        uint8_t* devEUI_ptr = (uint8_t*)devEUI;
        devEUI_ptr[0] = baseMac[0];
        devEUI_ptr[1] = baseMac[1];
        devEUI_ptr[2] = baseMac[2];
        devEUI_ptr[3] = 0xFF;  // Separador
        devEUI_ptr[4] = 0xFE;  // Separador
        devEUI_ptr[5] = baseMac[3];
        devEUI_ptr[6] = baseMac[4];
        devEUI_ptr[7] = baseMac[5];
        
        Serial.print("📱 DevEUI generado: ");
        for (int i = 0; i < 8; i++) {
            Serial.printf("%02X", devEUI[i]);
            if (i < 7) Serial.print("-");
        }
        Serial.println();
        
        Serial.println("⚠️  IMPORTANTE: Registrar este DevEUI en el servidor LoRaWAN");
    }
}

// Función para mostrar claves (solo para debug - NO usar en producción)
#ifdef DEBUG_SHOW_KEYS
void printLoRaWANKeys() {
    Serial.println("🔑 CLAVES LORAWAN CONFIGURADAS:");
    Serial.println("⚠️  SOLO PARA DEBUG - NO MOSTRAR EN PRODUCCIÓN");
    
    #if USE_OTAA
    Serial.print("JoinEUI: ");
    for (int i = 0; i < 8; i++) {
        Serial.printf("%02X", joinEUI[i]);
    }
    Serial.println();
    
    Serial.print("DevEUI:  ");
    for (int i = 0; i < 8; i++) {
        Serial.printf("%02X", devEUI[i]);
    }
    Serial.println();
    
    Serial.println("AppKey:  [OCULTA POR SEGURIDAD]");
    Serial.println("NwkKey:  [OCULTA POR SEGURIDAD]");
    #else
    Serial.printf("DevAddr: %08X\n", devAddr);
    Serial.println("NwkSKey: [OCULTA POR SEGURIDAD]");
    Serial.println("AppSKey: [OCULTA POR SEGURIDAD]");
    #endif
    
    Serial.println("Región:  AU915");
    Serial.printf("Sub-banda: %d\n", LORAWAN_SUB_BAND);
}
#endif

// ============================================================================
// VALIDACIÓN DE CLAVES
// ============================================================================

bool validateLoRaWANKeys() {
    #if USE_OTAA
    // Verificar que no todas las claves sean cero
    bool devEUI_valid = false;
    bool appKey_valid = false;
    
    for (int i = 0; i < 8; i++) {
        if (devEUI[i] != 0) {
            devEUI_valid = true;
            break;
        }
    }
    
    for (int i = 0; i < 16; i++) {
        if (appKey[i] != 0) {
            appKey_valid = true;
            break;
        }
    }
    
    if (!devEUI_valid) {
        Serial.println("❌ ERROR: DevEUI está vacío");
        return false;
    }
    
    if (!appKey_valid) {
        Serial.println("❌ ERROR: AppKey está vacío");
        return false;
    }
    
    Serial.println("✅ Claves OTAA validadas correctamente");
    return true;
    
    #else
    // Validar claves ABP
    if (devAddr == 0) {
        Serial.println("❌ ERROR: DevAddr está vacío");
        return false;
    }
    
    bool nwkSKey_valid = false;
    bool appSKey_valid = false;
    
    for (int i = 0; i < 16; i++) {
        if (nwkSKey[i] != 0) nwkSKey_valid = true;
        if (appSKey[i] != 0) appSKey_valid = true;
    }
    
    if (!nwkSKey_valid || !appSKey_valid) {
        Serial.println("❌ ERROR: Claves ABP están vacías");
        return false;
    }
    
    Serial.println("✅ Claves ABP validadas correctamente");
    return true;
    #endif
}

// ============================================================================
// INSTRUCCIONES DE CONFIGURACIÓN
// ============================================================================

/*
GUÍA PASO A PASO PARA CONFIGURAR CLAVES:

1. ELEGIR PROVEEDOR LORAWAN:
   - The Things Network (TTN): Gratuito, bueno para testing
   - ChirpStack: Servidor privado, mayor control
   - Helium: Red pública con tokenomics
   - Operador comercial: SLA profesional

2. REGISTRAR DISPOSITIVO:
   - Crear cuenta en proveedor elegido
   - Crear aplicación
   - Crear dispositivo con perfil AU915
   - Copiar claves generadas

3. CONFIGURAR ESTE ARCHIVO:
   - Pegar claves en las variables correspondientes
   - Verificar que USE_OTAA = 1 (recomendado)
   - Confirmar LORAWAN_SUB_BAND = 2 para Chile

4. COMPILAR Y CARGAR:
   - Compilar firmware: pio run
   - Cargar al collar: pio run --target upload
   - Monitorear JOIN: pio device monitor

5. VERIFICAR CONEXIÓN:
   - Ver logs de JOIN exitoso
   - Verificar recepción en servidor
   - Probar downlinks básicos

NOTAS DE SEGURIDAD:
- NUNCA subir claves a repositorios públicos
- Usar .gitignore para excluir este archivo si es necesario
- Cada collar debe tener DevEUI y AppKey únicos
- Rotar claves periódicamente en producción

TROUBLESHOOTING:
- JOIN falla: Verificar claves y alcance de gateway
- Sin downlinks: Verificar configuración RX windows
- Duplicate DevEUI: Cada collar necesita DevEUI único
*/

#endif // LORAWAN_KEYS_H