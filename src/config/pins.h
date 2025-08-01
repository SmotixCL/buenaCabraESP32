#pragma once

/*
 * ============================================================================
 * CONFIGURACIÓN DE PINES - HELTEC WIFI LORA 32 V3
 * ============================================================================
 * Hardware: ESP32-S3 + SX1262 + OLED 128x64
 * Versión: V3.2 (verificado con pinout oficial)
 */

// ============================================================================
// LORA SX1262 - PINS ESPECÍFICOS HELTEC V3
// ============================================================================
#define LORA_NSS    8     // Chip Select (SS)
#define LORA_RST    12    // Reset
#define LORA_DIO1   14    // DIO1/IRQ (interrupt)
#define LORA_BUSY   13    // BUSY status
#define LORA_SCK    9     // SPI Clock
#define LORA_MISO   11    // SPI Master Input
#define LORA_MOSI   10    // SPI Master Output

// ============================================================================
// OLED DISPLAY - PINS CORREGIDOS PARA V3
// ============================================================================
#define VEXT_ENABLE  36    // Control de alimentación OLED/Periféricos
#define VEXT_ON_VALUE LOW  // Valor para activar VEXT (LOW = ON)
#define OLED_SDA    17    // I2C Data
#define OLED_SCL    18    // I2C Clock  
#define OLED_RST    21    // Reset
#define OLED_ADDR   0x3C  // I2C Address

// ============================================================================
// CONTROL Y INTERFAZ
// ============================================================================
#define LED_PIN     35    // LED integrado (built-in)
#define BUZZER_PIN  7     // Buzzer pasivo PWM
#define VBAT_PIN    1     // ADC para medición batería

// ============================================================================
// GPS (UART) - PINES REALES SOLDADOS
// ============================================================================
#define GPS_TX_PIN  4     // TX hacia GPS (pin 4 en tu placa)
#define GPS_RX_PIN  3     // RX desde GPS (pin 3 en tu placa)
#define GPS_BAUD    9600  // Baudrate estándar GPS

// ============================================================================
// EXPANSIÓN FUTURA
// ============================================================================
#define EXP_PIN_1   32    // Pin expansión 1
#define EXP_PIN_2   33    // Pin expansión 2  
#define EXP_PIN_3   38    // Pin expansión 3 (disponible)
#define EXP_PIN_4   37    // Pin expansión 4
// Nota: Pin 36 usado como VEXT_ENABLE

// ============================================================================
// CONFIGURACIÓN SPI/I2C
// ============================================================================
#define SPI_FREQUENCY   8000000   // 8 MHz para SX1262
#define I2C_FREQUENCY   400000    // 400 kHz para OLED
#define SERIAL_BAUD     115200    // Debug serial
#define SERIAL_TIMEOUT  5000      // Timeout inicialización serial

// ============================================================================
// VALIDACIÓN DE HARDWARE
// ============================================================================
// Comentado para evitar warnings innecesarios durante compilación
// #ifndef ARDUINO_HELTEC_WIFI_LORA_32_V3
//     #warning "Este código está optimizado para Heltec WiFi LoRa 32 V3"
// #endif

// Verificar que los pines no entren en conflicto
#if LORA_NSS == OLED_SDA || LORA_NSS == OLED_SCL
    #error "Conflicto de pines entre LoRa y OLED"
#endif

// ============================================================================
// NOTAS IMPORTANTES
// ============================================================================
/*
 * 📌 PINES IMPORTANTES V3:
 * - NO usar GPIO 0, 19, 20 (reservados para boot)
 * - GPIO 1-21: disponibles con cuidado
 * - GPIO 35: LED built-in
 * - ADC1: mejores pines para analog read
 * 
 * 🔧 VERIFICADO EN HARDWARE:
 * ✅ LORA: 8,9,10,11,12,13,14 (funcionando)
 * ✅ OLED: 17,18,21 (funcionando)
 * ✅ BUZZER: 7 (PWM funcionando)
 * ✅ VBAT: 1 (ADC funcionando)
 */
