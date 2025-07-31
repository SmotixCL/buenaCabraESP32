/**
 * ============================================================================
 * COLLAR GEOFENCING - IMPLEMENTACIÓN HARDWARE
 * ============================================================================
 * 
 * @file Hardware.cpp
 * @version 3.0
 */

#include "Hardware.h"
#include <SPI.h>
#include <Wire.h>
#include <esp_system.h>
#include <esp_task_wdt.h>

// Variables estáticas
HardwareStatus Hardware::status = {false, false, false, false, SYSTEM_INIT};
bool Hardware::initialized = false;

bool Hardware::init() {
    DEBUG_INFO("🔧 Inicializando hardware...");
    
    // Inicializar pins básicos
    if (!initPins()) {
        DEBUG_ERROR("❌ Error inicializando pins");
        return false;
    }
    
    // Inicializar comunicaciones
    if (!initSerial()) {
        DEBUG_ERROR("❌ Error inicializando Serial");
        return false;
    }
    
    if (!initSPI()) {
        DEBUG_ERROR("❌ Error inicializando SPI");
        return false;
    }
    
    if (!initI2C()) {
        DEBUG_ERROR("❌ Error inicializando I2C");
        return false;
    }
    
    // Test básico de hardware
    ledBlink(3, 100);
    
    status.system_state = SYSTEM_RUNNING;
    initialized = true;
    
    DEBUG_INFO("✅ Hardware inicializado correctamente");
    printSystemInfo();
    
    return true;
}

bool Hardware::initPins() {
    DEBUG_INFO("📌 Configurando pins...");
    
    // Pins de salida
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(EXP_PIN_1, OUTPUT);
    pinMode(EXP_PIN_2, OUTPUT);
    pinMode(OLED_RST, OUTPUT);
    pinMode(LORA_RST, OUTPUT);
    
    // Pins de entrada
    pinMode(VBAT_PIN, INPUT);
    
    // Estados iniciales
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(EXP_PIN_1, LOW);
    digitalWrite(EXP_PIN_2, LOW);
    digitalWrite(OLED_RST, HIGH);
    digitalWrite(LORA_RST, HIGH);
    
    DEBUG_INFO("✅ Pins configurados");
    return true;
}

bool Hardware::initSPI() {
    DEBUG_INFO("🔄 Inicializando SPI...");
    
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    SPI.setFrequency(8000000); // 8 MHz
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    
    DEBUG_INFO("✅ SPI inicializado (8MHz, Mode0)");
    return true;
}

bool Hardware::initI2C() {
    DEBUG_INFO("🔄 Inicializando I2C...");
    
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000); // 400kHz
    
    // Test I2C - escanear dispositivos
    int devices = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            devices++;
            DEBUG_INFO("📍 Dispositivo I2C encontrado: 0x" + String(address, HEX));
        }
    }
    
    DEBUG_INFO("✅ I2C inicializado (" + String(devices) + " dispositivos)");
    return true;
}

bool Hardware::initSerial() {
    Serial.begin(SERIAL_BAUD_RATE);
    
    // Esperar conexión o timeout
    uint32_t start = millis();
    while (!Serial && (millis() - start) < SERIAL_TIMEOUT) {
        delay(10);
    }
    
    DEBUG_INFO("✅ Serial inicializado (" + String(SERIAL_BAUD_RATE) + " baud)");
    return true;
}

void Hardware::ledOn() {
    digitalWrite(LED_PIN, HIGH);
}

void Hardware::ledOff() {
    digitalWrite(LED_PIN, LOW);
}

void Hardware::ledBlink(uint8_t times, uint16_t duration) {
    for (uint8_t i = 0; i < times; i++) {
        ledOn();
        delay(duration);
        ledOff();
        if (i < times - 1) {
            delay(duration);
        }
    }
}

void Hardware::ledToggle() {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

HardwareStatus Hardware::getStatus() {
    return status;
}

bool Hardware::isInitialized() {
    return initialized;
}

void Hardware::printSystemInfo() {
    Serial.println(F("\n🔍 === INFORMACIÓN DEL SISTEMA ==="));
    Serial.println(F("📱 Hardware: Heltec WiFi LoRa 32 V3"));
    Serial.println(F("🧠 MCU: ESP32-S3FN8"));
    Serial.println(F("⚡ Frecuencia: " + String(getCpuFrequencyMhz()) + " MHz"));
    Serial.println(F("💾 RAM libre: " + String(getFreeHeap()) + " bytes"));
    Serial.println(F("📡 Flash: " + String(ESP.getFlashChipSize()) + " bytes"));
    Serial.println(F("🔧 Firmware: " + String(FIRMWARE_VERSION)));
    Serial.println(F("📅 Compilado: " + String(BUILD_DATE) + " " + String(BUILD_TIME)));
    Serial.println(F("⏱️ Uptime: " + String(getUptime()) + " segundos"));
    Serial.println(F("🌡️ CPU Temp: " + String(getCPUTemperature()) + "°C"));
    Serial.println(F("=================================\n"));
}

void Hardware::printPinMap() {
    Serial.println(F("\n📌 === MAPA DE PINS ==="));
    Serial.println(F("LoRa SX1262:"));
    Serial.println(F("  NSS:  " + String(LORA_NSS)));
    Serial.println(F("  RST:  " + String(LORA_RST)));
    Serial.println(F("  DIO1: " + String(LORA_DIO1)));
    Serial.println(F("  BUSY: " + String(LORA_BUSY)));
    Serial.println(F("  SCK:  " + String(LORA_SCK)));
    Serial.println(F("  MISO: " + String(LORA_MISO)));
    Serial.println(F("  MOSI: " + String(LORA_MOSI)));
    
    Serial.println(F("OLED:"));
    Serial.println(F("  SDA:  " + String(OLED_SDA)));
    Serial.println(F("  SCL:  " + String(OLED_SCL)));
    Serial.println(F("  RST:  " + String(OLED_RST)));
    
    Serial.println(F("Control:"));
    Serial.println(F("  LED:    " + String(LED_PIN)));
    Serial.println(F("  BUZZER: " + String(BUZZER_PIN)));
    Serial.println(F("  VBAT:   " + String(VBAT_PIN)));
    Serial.println(F("=======================\n"));
}

uint32_t Hardware::getUptime() {
    return millis() / 1000;
}

uint32_t Hardware::getFreeHeap() {
    return ESP.getFreeHeap();
}

float Hardware::getCPUTemperature() {
    // ESP32-S3 no tiene sensor de temperatura interno
    // Retornar valor estimado
    return 25.0;
}

void Hardware::enableLowPowerMode() {
    DEBUG_INFO("🔋 Activando modo bajo consumo");
    
    // Reducir frecuencia CPU
    setCpuFrequencyMhz(80);
    
    // Configurar GPIO para bajo consumo
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << EXP_PIN_1) | (1ULL << EXP_PIN_2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&config);
    
    status.system_state = SYSTEM_LOW_POWER;
}

void Hardware::disableLowPowerMode() {
    DEBUG_INFO("⚡ Desactivando modo bajo consumo");
    
    // Restaurar frecuencia CPU
    setCpuFrequencyMhz(240);
    
    status.system_state = SYSTEM_RUNNING;
}

void Hardware::prepareForSleep() {
    DEBUG_INFO("😴 Preparando para deep sleep");
    
    // Apagar periféricos
    ledOff();
    digitalWrite(BUZZER_PIN, LOW);
    
    // Configurar pins para sleep
    gpio_deep_sleep_hold_en();
}

void Hardware::wakeFromSleep() {
    DEBUG_INFO("⏰ Despertando de deep sleep");
    
    gpio_deep_sleep_hold_dis();
    
    // Reinicializar hardware crítico
    initPins();
}

void Hardware::feedWatchdog() {
    esp_task_wdt_reset();
}

void Hardware::enableWatchdog() {
    esp_task_wdt_init(30, true);  // 30 segundos
    esp_task_wdt_add(NULL);
}

void Hardware::disableWatchdog() {
    esp_task_wdt_delete(NULL);
    esp_task_wdt_deinit();
}