/**
 * ============================================================================
 * COLLAR BUENACABRA V3.0 - CONFIGURACIÓN DE PINES
 * ============================================================================
 * Definición de pines para Heltec WiFi LoRa 32 V3
 * 
 * @file pins.h
 * @version 3.0.0
 */

#ifndef PINS_H
#define PINS_H

// ============================================================================
// PINES LORA SX1262
// ============================================================================
#define LORA_NSS    8
#define LORA_RST    12
#define LORA_DIO1   14
#define LORA_BUSY   13
#define LORA_SCK    9
#define LORA_MISO   11
#define LORA_MOSI   10

// ============================================================================
// PINES OLED
// ============================================================================
#define OLED_SDA    17
#define OLED_SCL    18
#define OLED_RST    21
#define OLED_ADDR   0x3C

// ============================================================================
// PINES DE CONTROL
// ============================================================================
#define LED_PIN     35
#define BUZZER_PIN  7
#define VBAT_PIN    1
#define PRG_BUTTON  0

// ============================================================================
// CONTROL DE ALIMENTACIÓN
// ============================================================================
#define VEXT_ENABLE 36
#define VEXT_ON_VALUE LOW  // IMPORTANTE: LOW (0) activa la alimentación en Heltec V3

// ============================================================================
// PINES GPS
// ============================================================================
#define GPS_RX_PIN  3
#define GPS_TX_PIN  4

// ============================================================================
// PINES DE EXPANSIÓN (NO USADOS)
// ============================================================================
#define EXP_PIN_1   2
#define EXP_PIN_2   5

#endif // PINS_H
