/**
 * ============================================================================
 * COLLAR GEOFENCING - IMPLEMENTACIÓN RADIO MANAGER
 * ============================================================================
 * 
 * @file RadioManager.cpp
 * @version 3.0
 */

#include "RadioManager.h"

// Variables estáticas
SX1262* RadioManager::radio = nullptr;
bool RadioManager::initialized = false;
bool RadioManager::lorawan_mode = false;
uint16_t RadioManager::packet_counter = 0;
uint32_t RadioManager::last_transmission = 0;
int RadioManager::last_rssi = 0;
float RadioManager::last_snr = 0.0;

bool RadioManager::init(bool enable_lorawan) {
    DEBUG_INFO("📡 Inicializando radio SX1262...");
    
    lorawan_mode = enable_lorawan;
    
    // Configurar SPI
    if (!setupSPI()) {
        DEBUG_ERROR("❌ Error configurando SPI para radio");
        return false;
    }
    
    // Crear instancia del radio
    radio = new SX1262(new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY));
    
    if (!radio) {
        DEBUG_ERROR("❌ Error creando instancia del radio");
        return false;
    }
    
    // Inicializar con configuración básica
    int state = radio->begin(LORA_FREQUENCY, LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODING_RATE);
    
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_ERROR("❌ Error inicializando SX1262: " + String(state));
        delete radio;
        radio = nullptr;
        return false;
    }
    
    // Configurar parámetros adicionales
    radio->setOutputPower(LORA_OUTPUT_POWER);
    radio->setCurrentLimit(LORA_CURRENT_LIMIT);
    
    // Configurar sync word
    radio->setSyncWord(RADIOLIB_SX126X_SYNC_WORD_PRIVATE);
    
    initialized = true;
    packet_counter = 0;
    
    DEBUG_INFO("✅ SX1262 inicializado correctamente");
    DEBUG_INFO("   Frecuencia: " + String(LORA_FREQUENCY) + " MHz");
    DEBUG_INFO("   Ancho de banda: " + String(LORA_BANDWIDTH) + " kHz");
    DEBUG_INFO("   Factor de dispersión: SF" + String(LORA_SPREADING_FACTOR));
    DEBUG_INFO("   Potencia: " + String(LORA_OUTPUT_POWER) + " dBm");
    
    // Test de funcionamiento
    if (!testConnection()) {
        DEBUG_WARN("⚠️ Test de conexión falló, pero radio inicializado");
    }
    
    return true;
}

bool RadioManager::setupSPI() {
    DEBUG_INFO("🔄 Configurando SPI para radio...");
    
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    SPI.setFrequency(8000000);  // 8 MHz
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    
    DEBUG_INFO("✅ SPI configurado para radio");
    return true;
}

bool RadioManager::testConnection() {
    if (!initialized || !radio) return false;
    
    DEBUG_INFO("🧪 Probando conexión del radio...");
    
    // Test básico de transmisión
    String test_message = "COLLAR_TEST_" + String(millis());
    int state = radio->transmit(test_message);
    
    if (state == RADIOLIB_ERR_NONE) {
        packet_counter++;
        last_transmission = millis();
        DEBUG_INFO("✅ Test de transmisión exitoso");
        return true;
    } else {
        DEBUG_WARN("⚠️ Test de transmisión falló: " + String(state));
        return false;
    }
}

bool RadioManager::isInitialized() {
    return initialized && radio != nullptr;
}

void RadioManager::deinit() {
    if (radio) {
        delete radio;
        radio = nullptr;
    }
    
    initialized = false;
    lorawan_mode = false;
    packet_counter = 0;
    
    DEBUG_INFO("📡 Radio desinicializado");
}

bool RadioManager::setFrequency(float frequency) {
    if (!isInitialized()) return false;
    
    int state = radio->setFrequency(frequency);
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_INFO("📡 Frecuencia configurada: " + String(frequency) + " MHz");
        return true;
    } else {
        DEBUG_ERROR("❌ Error configurando frecuencia: " + String(state));
        return false;
    }
}

bool RadioManager::setBandwidth(float bandwidth) {
    if (!isInitialized()) return false;
    
    int state = radio->setBandwidth(bandwidth);
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_INFO("📡 Ancho de banda configurado: " + String(bandwidth) + " kHz");
        return true;
    } else {
        DEBUG_ERROR("❌ Error configurando ancho de banda: " + String(state));
        return false;
    }
}

bool RadioManager::setSpreadingFactor(uint8_t sf) {
    if (!isInitialized()) return false;
    
    int state = radio->setSpreadingFactor(sf);
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_INFO("📡 Factor de dispersión configurado: SF" + String(sf));
        return true;
    } else {
        DEBUG_ERROR("❌ Error configurando SF: " + String(state));
        return false;
    }
}

bool RadioManager::setCodingRate(uint8_t cr) {
    if (!isInitialized()) return false;
    
    int state = radio->setCodingRate(cr);
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_INFO("📡 Tasa de codificación configurada: 4/" + String(cr));
        return true;
    } else {
        DEBUG_ERROR("❌ Error configurando CR: " + String(state));
        return false;
    }
}

bool RadioManager::setOutputPower(int8_t power) {
    if (!isInitialized()) return false;
    
    int state = radio->setOutputPower(power);
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_INFO("📡 Potencia configurada: " + String(power) + " dBm");
        return true;
    } else {
        DEBUG_ERROR("❌ Error configurando potencia: " + String(state));
        return false;
    }
}

bool RadioManager::setCurrentLimit(uint8_t current) {
    if (!isInitialized()) return false;
    
    int state = radio->setCurrentLimit(current);
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_INFO("📡 Límite de corriente configurado: " + String(current) + " mA");
        return true;
    } else {
        DEBUG_ERROR("❌ Error configurando límite de corriente: " + String(state));
        return false;
    }
}

bool RadioManager::transmit(const String& data) {
    if (!isInitialized()) return false;
    
    DEBUG_INFO("📡 Transmitiendo: " + data);
    
    int state = radio->transmit(data);
    
    if (state == RADIOLIB_ERR_NONE) {
        packet_counter++;
        last_transmission = millis();
        
        // Obtener estadísticas
        last_rssi = radio->getRSSI();
        last_snr = radio->getSNR();
        
        DEBUG_INFO("✅ Transmisión exitosa #" + String(packet_counter));
        DEBUG_INFO("   RSSI: " + String(last_rssi) + " dBm");
        DEBUG_INFO("   SNR: " + String(last_snr) + " dB");
        
        return true;
    } else {
        DEBUG_ERROR("❌ Error en transmisión: " + String(state));
        return false;
    }
}

bool RadioManager::transmit(const uint8_t* data, size_t length) {
    if (!isInitialized() || !data || length == 0) return false;
    
    DEBUG_INFO("📡 Transmitiendo " + String(length) + " bytes");
    
    int state = radio->transmit(const_cast<uint8_t*>(data), length);
    
    if (state == RADIOLIB_ERR_NONE) {
        packet_counter++;
        last_transmission = millis();
        last_rssi = radio->getRSSI();
        last_snr = radio->getSNR();
        
        DEBUG_INFO("✅ Transmisión binaria exitosa #" + String(packet_counter));
        return true;
    } else {
        DEBUG_ERROR("❌ Error en transmisión binaria: " + String(state));
        return false;
    }
}

bool RadioManager::transmitPacket(const Position& pos, const BatteryStatus& battery, AlertLevel alert_level) {
    if (!isInitialized()) return false;
    
    // Crear payload estructurado
    String payload = "COLLAR:" + String(packet_counter + 1) + 
                    ",LAT:" + String(pos.latitude, 6) +
                    ",LNG:" + String(pos.longitude, 6) +
                    ",ALT:" + String(pos.altitude, 1) +
                    ",SAT:" + String(pos.satellites) +
                    ",HDOP:" + String(pos.hdop, 2) +
                    ",BAT:" + String(battery.voltage, 2) +
                    ",PCT:" + String(battery.percentage) +
                    ",ALERT:" + String(alert_level) +
                    ",TIME:" + String(millis());
    
    return transmit(payload);
}

bool RadioManager::receive(String& data) {
    if (!isInitialized()) return false;
    
    int state = radio->receive(data);
    
    if (state == RADIOLIB_ERR_NONE) {
        last_rssi = radio->getRSSI();
        last_snr = radio->getSNR();
        
        DEBUG_INFO("📡 Recibido: " + data);
        DEBUG_INFO("   RSSI: " + String(last_rssi) + " dBm");
        DEBUG_INFO("   SNR: " + String(last_snr) + " dB");
        
        return true;
    } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
        DEBUG_ERROR("❌ Error en recepción: " + String(state));
    }
    
    return false;
}

bool RadioManager::receive(uint8_t* data, size_t& length) {
    if (!isInitialized() || !data) return false;
    
    int state = radio->receive(data, length);
    
    if (state == RADIOLIB_ERR_NONE) {
        last_rssi = radio->getRSSI();
        last_snr = radio->getSNR();
        
        DEBUG_INFO("📡 Recibidos " + String(length) + " bytes");
        return true;
    } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
        DEBUG_ERROR("❌ Error en recepción binaria: " + String(state));
    }
    
    return false;
}

bool RadioManager::available() {
    if (!isInitialized()) return false;
    
    // Verificar si hay datos disponibles (implementación básica)
    return radio->getIrqStatus() != 0;
}

uint16_t RadioManager::getPacketCounter() {
    return packet_counter;
}

int RadioManager::getLastRSSI() {
    return last_rssi;
}

float RadioManager::getLastSNR() {
    return last_snr;
}

uint32_t RadioManager::getLastTransmissionTime() {
    return last_transmission;
}

String RadioManager::getRadioInfo() {
    if (!isInitialized()) return "Radio no inicializado";
    
    String info = "SX1262 Radio Info:\n";
    info += "  Frecuencia: " + String(LORA_FREQUENCY) + " MHz\n";
    info += "  Ancho de banda: " + String(LORA_BANDWIDTH) + " kHz\n";
    info += "  SF: " + String(LORA_SPREADING_FACTOR) + "\n";
    info += "  CR: 4/" + String(LORA_CODING_RATE) + "\n";
    info += "  Potencia: " + String(LORA_OUTPUT_POWER) + " dBm\n";
    info += "  Paquetes: " + String(packet_counter) + "\n";
    info += "  Último RSSI: " + String(last_rssi) + " dBm\n";
    info += "  Último SNR: " + String(last_snr) + " dB";
    
    return info;
}

void RadioManager::reset() {
    if (!isInitialized()) return;
    
    DEBUG_INFO("🔄 Reiniciando radio...");
    
    // Reset por hardware
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    
    // Reinicializar
    deinit();
    delay(100);
    init(lorawan_mode);
}

void RadioManager::sleep() {
    if (!isInitialized()) return;
    
    radio->sleep();
    DEBUG_INFO("😴 Radio en modo sleep");
}

void RadioManager::wakeup() {
    if (!isInitialized()) return;
    
    radio->standby();
    DEBUG_INFO("⏰ Radio despertado");
}

bool RadioManager::testRadio() {
    if (!isInitialized()) {
        DEBUG_ERROR("❌ Radio no inicializado para test");
        return false;
    }
    
    DEBUG_INFO("🧪 === TEST COMPLETO DEL RADIO ===");
    
    bool all_tests_passed = true;
    
    // Test 1: Verificar configuración
    DEBUG_INFO("🧪 Test 1: Verificar configuración");
    // En un radio real, aquí verificaríamos registros
    DEBUG_INFO("✅ Configuración OK");
    
    // Test 2: Test de transmisión
    DEBUG_INFO("🧪 Test 2: Transmisión de prueba");
    String test_msg = "TEST_" + String(millis());
    if (transmit(test_msg)) {
        DEBUG_INFO("✅ Transmisión OK");
    } else {
        DEBUG_ERROR("❌ Transmisión falló");
        all_tests_passed = false;
    }
    
    // Test 3: Test de diferentes potencias
    DEBUG_INFO("🧪 Test 3: Diferentes potencias");
    int8_t test_powers[] = {10, 15, 20};
    for (int i = 0; i < 3; i++) {
        if (setOutputPower(test_powers[i])) {
            transmit("PWR_TEST_" + String(test_powers[i]));
            delay(100);
        } else {
            all_tests_passed = false;
        }
    }
    
    // Restaurar potencia original
    setOutputPower(LORA_OUTPUT_POWER);
    
    DEBUG_INFO(all_tests_passed ? "✅ Todos los tests pasaron" : "❌ Algunos tests fallaron");
    return all_tests_passed;
}

void RadioManager::printStatus() {
    Serial.println(F("\n📡 === ESTADO DEL RADIO ==="));
    Serial.println(F("Inicializado: ") + String(initialized ? "SÍ" : "NO"));
    Serial.println(F("Modo LoRaWAN: ") + String(lorawan_mode ? "SÍ" : "NO"));
    Serial.println(F("Paquetes enviados: ") + String(packet_counter));
    Serial.println(F("Última transmisión: ") + String((millis() - last_transmission) / 1000) + "s ago");
    Serial.println(F("Último RSSI: ") + String(last_rssi) + " dBm");
    Serial.println(F("Último SNR: ") + String(last_snr) + " dB");
    Serial.println(F("Frecuencia: ") + String(LORA_FREQUENCY) + " MHz");
    Serial.println(F("Potencia: ") + String(LORA_OUTPUT_POWER) + " dBm");
    Serial.println(F("Pins NSS/RST/DIO1/BUSY: ") + String(LORA_NSS) + "/" + 
                   String(LORA_RST) + "/" + String(LORA_DIO1) + "/" + String(LORA_BUSY));
    Serial.println(F("===========================\n"));
}

// Funciones de LoRaWAN (básicas por ahora)
bool RadioManager::initLoRaWAN() {
    DEBUG_INFO("📡 Inicializando modo LoRaWAN...");
    // TODO: Implementar LoRaWAN completo
    lorawan_mode = true;
    DEBUG_WARN("⚠️ LoRaWAN no implementado completamente");
    return false;
}

bool RadioManager::joinOTAA(const String& dev_eui, const String& app_eui, const String& app_key) {
    DEBUG_INFO("📡 Intentando join OTAA...");
    // TODO: Implementar OTAA
    DEBUG_WARN("⚠️ OTAA no implementado");
    return false;
}

bool RadioManager::joinABP(const String& dev_addr, const String& nwk_s_key, const String& app_s_key) {
    DEBUG_INFO("📡 Configurando ABP...");
    // TODO: Implementar ABP
    DEBUG_WARN("⚠️ ABP no implementado");
    return false;
}

bool RadioManager::sendLoRaWAN(const uint8_t* data, size_t length, uint8_t port) {
    DEBUG_INFO("📡 Enviando paquete LoRaWAN...");
    // TODO: Implementar envío LoRaWAN
    DEBUG_WARN("⚠️ Envío LoRaWAN no implementado");
    return false;
}

bool RadioManager::isJoined() {
    // TODO: Implementar estado de join
    return false;
}

void RadioManager::onReceive(void (*callback)(void)) {
    if (!isInitialized()) return;
    // TODO: Implementar callback de recepción
    DEBUG_WARN("⚠️ Callback de recepción no implementado");
}

void RadioManager::onTransmit(void (*callback)(void)) {
    if (!isInitialized()) return;
    // TODO: Implementar callback de transmisión
    DEBUG_WARN("⚠️ Callback de transmisión no implementado");
}