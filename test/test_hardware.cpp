/**
 * ============================================================================
 * TEST UNITARIO - HARDWARE CHECK
 * ============================================================================
 * Test básico para verificar el funcionamiento del hardware
 * 
 * @file test_hardware.cpp
 * @version 3.0.0
 */

#include <Arduino.h>
#include <unity.h>
#include <Wire.h>
#include "../include/config.h"

// ============================================================================
// TESTS DE PINES
// ============================================================================

void test_led_pin() {
    pinMode(LED_PIN, OUTPUT);
    
    // Test encender LED
    digitalWrite(LED_PIN, HIGH);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(LED_PIN));
    
    // Test apagar LED
    digitalWrite(LED_PIN, LOW);
    TEST_ASSERT_EQUAL(LOW, digitalRead(LED_PIN));
}

void test_button_pin() {
    pinMode(PRG_BUTTON, INPUT_PULLUP);
    
    // El botón debe estar HIGH cuando no está presionado (pull-up)
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PRG_BUTTON));
}

void test_vext_control() {
    pinMode(VEXT_ENABLE, OUTPUT);
    
    // Test activar VEXT
    digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE);
    TEST_ASSERT_EQUAL(VEXT_ON_VALUE, digitalRead(VEXT_ENABLE));
    
    // Test desactivar VEXT
    digitalWrite(VEXT_ENABLE, !VEXT_ON_VALUE);
    TEST_ASSERT_EQUAL(!VEXT_ON_VALUE, digitalRead(VEXT_ENABLE));
}

// ============================================================================
// TESTS DE COMUNICACIÓN
// ============================================================================

void test_serial_communication() {
    // Verificar que Serial esté disponible
    TEST_ASSERT_TRUE(Serial);
    
    // Test baudrate
    TEST_ASSERT_EQUAL(SERIAL_BAUD, Serial.baudRate());
}

void test_i2c_bus() {
    Wire.begin(OLED_SDA, OLED_SCL);
    
    // Escanear bus I2C
    uint8_t devices_found = 0;
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            devices_found++;
        }
    }
    
    // Debería encontrar al menos el OLED (0x3C)
    TEST_ASSERT_GREATER_OR_EQUAL(1, devices_found);
}

// ============================================================================
// TESTS DE MEMORIA
// ============================================================================

void test_heap_memory() {
    uint32_t free_heap = ESP.getFreeHeap();
    
    // Debe haber al menos 10KB libres
    TEST_ASSERT_GREATER_THAN(10000, free_heap);
    
    // Test de asignación dinámica
    uint8_t* test_buffer = (uint8_t*)malloc(1024);
    TEST_ASSERT_NOT_NULL(test_buffer);
    
    // Liberar memoria
    free(test_buffer);
    
    // Verificar que la memoria se liberó correctamente
    uint32_t heap_after = ESP.getFreeHeap();
    TEST_ASSERT_GREATER_OR_EQUAL(free_heap - 100, heap_after); // Tolerancia de 100 bytes
}

void test_flash_size() {
    uint32_t flash_size = ESP.getFlashChipSize();
    
    // ESP32-S3 debe tener al menos 4MB de flash
    TEST_ASSERT_GREATER_OR_EQUAL(4 * 1024 * 1024, flash_size);
}

// ============================================================================
// TESTS DE CONFIGURACIÓN
// ============================================================================

void test_config_values() {
    // Verificar que los valores de configuración sean válidos
    TEST_ASSERT_GREATER_THAN(0, SERIAL_BAUD);
    TEST_ASSERT_GREATER_THAN(0, GPS_BAUD_RATE);
    TEST_ASSERT_GREATER_THAN(0, I2C_FREQUENCY);
    
    // Verificar rangos de pines
    TEST_ASSERT_LESS_THAN(40, LED_PIN);
    TEST_ASSERT_LESS_THAN(40, BUZZER_PIN);
    TEST_ASSERT_LESS_THAN(40, PRG_BUTTON);
    
    // Verificar valores de LoRaWAN
    TEST_ASSERT_GREATER_THAN(0, LORAWAN_FREQUENCY);
    TEST_ASSERT_GREATER_THAN(0, LORAWAN_BANDWIDTH);
    TEST_ASSERT_GREATER_THAN(0, LORAWAN_SF);
}

void test_battery_adc() {
    // Configurar ADC para batería
    pinMode(VBAT_PIN, INPUT);
    
    // Leer valor ADC
    int adc_value = analogRead(VBAT_PIN);
    
    // El valor debe estar en el rango válido del ADC (0-4095 para 12 bits)
    TEST_ASSERT_GREATER_OR_EQUAL(0, adc_value);
    TEST_ASSERT_LESS_OR_EQUAL(4095, adc_value);
}

// ============================================================================
// TESTS DE TIEMPO
// ============================================================================

void test_millis_function() {
    uint32_t start = millis();
    delay(100);
    uint32_t end = millis();
    
    // Debe haber pasado aproximadamente 100ms (tolerancia de 10ms)
    uint32_t elapsed = end - start;
    TEST_ASSERT_GREATER_OR_EQUAL(90, elapsed);
    TEST_ASSERT_LESS_OR_EQUAL(110, elapsed);
}

void test_micros_function() {
    uint32_t start = micros();
    delayMicroseconds(1000);
    uint32_t end = micros();
    
    // Debe haber pasado aproximadamente 1000us (tolerancia de 100us)
    uint32_t elapsed = end - start;
    TEST_ASSERT_GREATER_OR_EQUAL(900, elapsed);
    TEST_ASSERT_LESS_OR_EQUAL(1100, elapsed);
}

// ============================================================================
// RUNNER DE TESTS
// ============================================================================

void setup() {
    // Inicializar Serial para output de Unity
    Serial.begin(SERIAL_BAUD);
    
    // Esperar a que Serial esté listo
    uint32_t timeout = millis() + 5000;
    while (!Serial && millis() < timeout) {
        delay(10);
    }
    
    delay(2000); // Esperar para estabilización
    
    // Comenzar tests después de 2 segundos
    UNITY_BEGIN();
    
    // Tests de pines
    RUN_TEST(test_led_pin);
    RUN_TEST(test_button_pin);
    RUN_TEST(test_vext_control);
    
    // Tests de comunicación
    RUN_TEST(test_serial_communication);
    RUN_TEST(test_i2c_bus);
    
    // Tests de memoria
    RUN_TEST(test_heap_memory);
    RUN_TEST(test_flash_size);
    
    // Tests de configuración
    RUN_TEST(test_config_values);
    RUN_TEST(test_battery_adc);
    
    // Tests de tiempo
    RUN_TEST(test_millis_function);
    RUN_TEST(test_micros_function);
    
    UNITY_END();
}

void loop() {
    // Los tests se ejecutan una sola vez en setup()
    delay(1000);
}
