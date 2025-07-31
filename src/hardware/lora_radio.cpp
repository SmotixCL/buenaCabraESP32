/*
 * ============================================================================
 * IMPLEMENTACIÓN MÓDULO RADIO LORA - Comunicaciones SX1262
 * ============================================================================
 */

#include "hardware/lora_radio.h"

// Variables externas
extern SystemStatus_t system_status;
extern AlertState_t alert_state;

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
LoRaController Radio;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================
LoRaController::LoRaController() 
    : radio(nullptr), initialized(false), packet_counter(0), 
      last_transmission(0), last_receive_check(0), last_rssi(0), last_snr(0.0) {
}

LoRaController::~LoRaController() {
    end();
}

// ============================================================================
// INICIALIZACIÓN Y CONFIGURACIÓN
// ============================================================================
bool LoRaController::setupSPI() {
    DEBUG_PRINTLN(F("📡 Configurando SPI para radio..."));
    
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    
    DEBUG_PRINTF("📡 SPI configurado: SCK=%d, MISO=%d, MOSI=%d, NSS=%d\n", 
                 LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    
    return true;
}

bool LoRaController::configureRadio() {
    DEBUG_PRINTLN(F("📡 Configurando parámetros del radio..."));
    
    // Configurar frecuencia
    int state = radio->setFrequency(LORA_FREQUENCY);
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("❌ Error configurando frecuencia: %d\n", state);
        return false;
    }
    
    // Configurar bandwidth
    state = radio->setBandwidth(LORA_BANDWIDTH);
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("❌ Error configurando bandwidth: %d\n", state);
        return false;
    }
    
    // Configurar spreading factor
    state = radio->setSpreadingFactor(LORA_SPREADING_FACTOR);
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("❌ Error configurando SF: %d\n", state);
        return false;
    }
    
    // Configurar coding rate
    state = radio->setCodingRate(LORA_CODING_RATE);
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("❌ Error configurando CR: %d\n", state);
        return false;
    }
    
    // Configurar sync word
    state = radio->setSyncWord(LORA_SYNC_WORD);
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("❌ Error configurando sync word: %d\n", state);
        return false;
    }
    
    // Configurar potencia de salida
    state = radio->setOutputPower(LORA_OUTPUT_POWER);
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("❌ Error configurando potencia: %d\n", state);
        return false;
    }
    
    // Configurar límite de corriente
    state = radio->setCurrentLimit(LORA_CURRENT_LIMIT);
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("❌ Error configurando límite corriente: %d\n", state);
        return false;
    }
    
    DEBUG_PRINTLN(F("✅ Parámetros del radio configurados correctamente"));
    return true;
}

bool LoRaController::begin() {
    DEBUG_PRINTLN(F("📡 Inicializando radio SX1262..."));
    
    // Configurar SPI
    if (!setupSPI()) {
        return false;
    }
    
    // Crear instancia del radio
    radio = new SX1262(new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY));
    
    if (!radio) {
        DEBUG_PRINTLN(F("❌ Error: No se pudo crear instancia del radio"));
        return false;
    }
    
    // Inicializar radio básico
    int state = radio->begin();
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("❌ Error inicializando SX1262: %d\n", state);
        delete radio;
        radio = nullptr;
        return false;
    }
    
    // Configurar parámetros
    if (!configureRadio()) {
        delete radio;
        radio = nullptr;
        return false;
    }
    
    initialized = true;
    packet_counter = 0;
    last_transmission = 0;
    
    DEBUG_PRINTLN(F("✅ SX1262 inicializado correctamente"));
    
    // Test básico de transmisión
    DEBUG_PRINTLN(F("📡 Enviando test packet..."));
    state = radio->transmit("COLLAR_V3_INIT_TEST");
    
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_PRINTLN(F("✅ Test packet enviado correctamente"));
        packet_counter = 1;
        last_transmission = millis();
    } else {
        DEBUG_PRINTF("⚠️ Test packet falló: %d\n", state);
    }
    
    return true;
}

void LoRaController::end() {
    if (!initialized) return;
    
    if (radio) {
        delete radio;
        radio = nullptr;
    }
    
    initialized = false;
    DEBUG_PRINTLN(F("📡 Radio desactivado"));
}

// ============================================================================
// UTILIDADES DE PAQUETES
// ============================================================================
uint8_t LoRaController::calculateChecksum(const uint8_t* data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length - 1; i++) { // Excluir el propio checksum
        checksum ^= data[i];
    }
    return checksum;
}

bool LoRaController::validatePacket(const LoRaPacket_t* packet) {
    if (!packet) return false;
    
    uint8_t calculated_checksum = calculateChecksum((const uint8_t*)packet, sizeof(LoRaPacket_t));
    return (calculated_checksum == packet->checksum);
}

bool LoRaController::validateDownlink(const LoRaDownlink_t* downlink) {
    if (!downlink) return false;
    
    uint8_t calculated_checksum = calculateChecksum((const uint8_t*)downlink, sizeof(LoRaDownlink_t));
    return (calculated_checksum == downlink->checksum);
}

// ============================================================================
// TRANSMISIÓN
// ============================================================================
bool LoRaController::sendPositionPacket(const Position_t& pos, uint8_t alert_level, float battery_voltage) {
    if (!initialized) return false;
    
    LoRaPacket_t packet;
    packet.packet_id = packet_counter + 1;
    packet.latitude = pos.latitude;
    packet.longitude = pos.longitude;
    packet.altitude = pos.altitude;
    packet.alert_level = alert_level;
    packet.battery_voltage = battery_voltage;
    packet.satellites = pos.satellites;
    packet.timestamp = millis();
    packet.checksum = calculateChecksum((const uint8_t*)&packet, sizeof(LoRaPacket_t));
    
    DEBUG_PRINTF("📡 Enviando packet #%d: lat=%.6f, lng=%.6f, alert=%d, bat=%.2fV\n", 
                 packet.packet_id, packet.latitude, packet.longitude, 
                 packet.alert_level, packet.battery_voltage);
    
    int state = radio->transmit((uint8_t*)&packet, sizeof(LoRaPacket_t));
    
    if (state == RADIOLIB_ERR_NONE) {
        packet_counter++;
        last_transmission = millis();
        
        // Obtener estadísticas de transmisión
        last_rssi = radio->getRSSI();
        last_snr = radio->getSNR();
        
        DEBUG_PRINTF("✅ Packet #%d enviado exitosamente (RSSI: %d dBm, SNR: %.2f dB)\n", 
                     packet_counter, last_rssi, last_snr);
        
        return true;
    } else {
        DEBUG_PRINTF("❌ Error enviando packet: %d\n", state);
        return false;
    }
}

bool LoRaController::sendHeartbeat() {
    if (!initialized) return false;
    
    String heartbeat = "HEARTBEAT:" + String(packet_counter + 1) + 
                      ",UPTIME:" + String(millis()) +
                      ",STATUS:OK";
    
    DEBUG_PRINTF("💓 Enviando heartbeat: %s\n", heartbeat.c_str());
    
    int state = radio->transmit(heartbeat);
    
    if (state == RADIOLIB_ERR_NONE) {
        packet_counter++;
        last_transmission = millis();
        DEBUG_PRINTLN(F("✅ Heartbeat enviado"));
        return true;
    } else {
        DEBUG_PRINTF("❌ Error enviando heartbeat: %d\n", state);
        return false;
    }
}

bool LoRaController::sendAlert(uint8_t alert_level, float distance) {
    if (!initialized) return false;
    
    String alert_msg = "ALERT:" + String(alert_level) + 
                      ",DIST:" + String(distance, 1) +
                      ",TIME:" + String(millis());
    
    DEBUG_PRINTF("🚨 Enviando alerta: %s\n", alert_msg.c_str());
    
    int state = radio->transmit(alert_msg);
    
    if (state == RADIOLIB_ERR_NONE) {
        packet_counter++;
        last_transmission = millis();
        DEBUG_PRINTLN(F("✅ Alerta enviada"));
        return true;
    } else {
        DEBUG_PRINTF("❌ Error enviando alerta: %d\n", state);
        return false;
    }
}

bool LoRaController::sendCustomPacket(const char* payload) {
    if (!initialized || !payload) return false;
    
    DEBUG_PRINTF("📡 Enviando packet custom: %s\n", payload);
    
    int state = radio->transmit(payload);
    
    if (state == RADIOLIB_ERR_NONE) {
        packet_counter++;
        last_transmission = millis();
        DEBUG_PRINTLN(F("✅ Packet custom enviado"));
        return true;
    } else {
        DEBUG_PRINTF("❌ Error enviando packet custom: %d\n", state);
        return false;
    }
}

// ============================================================================
// RECEPCIÓN Y DOWNLINKS
// ============================================================================
void LoRaController::checkDownlinks() {
    if (!initialized) return;
    
    uint32_t current_time = millis();
    if (current_time - last_receive_check < 1000) return; // Cada segundo
    
    // TODO: Implementar recepción de downlinks
    // Por ahora solo logging para debugging
    static uint8_t check_counter = 0;
    if (++check_counter >= 60) { // Cada minuto
        DEBUG_PRINTLN(F("📡 Verificando downlinks disponibles..."));
        check_counter = 0;
    }
    
    last_receive_check = current_time;
}

bool LoRaController::isDownlinkAvailable() {
    // TODO: Implementar detección de downlinks disponibles
    return false;
}

void LoRaController::processDownlink(const LoRaDownlink_t* downlink) {
    if (!downlink || !validateDownlink(downlink)) {
        DEBUG_PRINTLN(F("❌ Downlink inválido recibido"));
        return;
    }
    
    DEBUG_PRINTF("📥 Procesando downlink: cmd=0x%02X, param=0x%02X\n", 
                 downlink->command, downlink->parameter);
    
    switch (downlink->command) {
        case CMD_PING:
            DEBUG_PRINTLN(F("📥 Comando PING recibido"));
            sendCustomPacket("PONG");
            break;
            
        case CMD_TRIGGER_ALERT:
            DEBUG_PRINTLN(F("📥 Comando TRIGGER_ALERT recibido"));
            // Activar alerta manual
            break;
            
        case CMD_SET_INTERVAL:
            DEBUG_PRINTF("📥 Comando SET_INTERVAL recibido: %d\n", downlink->parameter);
            // Cambiar intervalo de transmisión
            break;
            
        case CMD_REBOOT:
            DEBUG_PRINTLN(F("📥 Comando REBOOT recibido"));
            delay(1000);
            ESP.restart();
            break;
            
        default:
            DEBUG_PRINTF("❌ Comando desconocido: 0x%02X\n", downlink->command);
            break;
    }
}

// ============================================================================
// CONFIGURACIÓN DINÁMICA
// ============================================================================
bool LoRaController::setFrequency(float frequency) {
    if (!initialized) return false;
    
    int state = radio->setFrequency(frequency);
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("✅ Frecuencia cambiada a %.1f MHz\n", frequency);
        return true;
    } else {
        DEBUG_PRINTF("❌ Error cambiando frecuencia: %d\n", state);
        return false;
    }
}

bool LoRaController::setOutputPower(int8_t power) {
    if (!initialized) return false;
    
    int state = radio->setOutputPower(power);
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("✅ Potencia cambiada a %d dBm\n", power);
        return true;
    } else {
        DEBUG_PRINTF("❌ Error cambiando potencia: %d\n", state);
        return false;
    }
}

// ============================================================================
// TEST Y DIAGNÓSTICO
// ============================================================================
void LoRaController::runSelfTest() {
    if (!initialized) {
        DEBUG_PRINTLN(F("❌ Radio no inicializado para self-test"));
        return;
    }
    
    DEBUG_PRINTLN(F("🧪 === SELF-TEST RADIO ==="));
    
    // Test de configuración
    printConfiguration();
    
    // Test de transmisión básica
    DEBUG_PRINTLN(F("📡 Test de transmisión básica..."));
    if (transmitTest()) {
        DEBUG_PRINTLN(F("✅ Test de transmisión: OK"));
    } else {
        DEBUG_PRINTLN(F("❌ Test de transmisión: FAIL"));
    }
    
    // Test de heartbeat
    DEBUG_PRINTLN(F("💓 Test de heartbeat..."));
    if (sendHeartbeat()) {
        DEBUG_PRINTLN(F("✅ Test de heartbeat: OK"));
    } else {
        DEBUG_PRINTLN(F("❌ Test de heartbeat: FAIL"));
    }
    
    DEBUG_PRINTLN(F("✅ Self-test radio completado"));
}

bool LoRaController::transmitTest() {
    if (!initialized) return false;
    
    String test_msg = "TEST_" + String(millis());
    return sendCustomPacket(test_msg.c_str());
}

void LoRaController::printConfiguration() {
    if (!initialized) {
        DEBUG_PRINTLN(F("❌ Radio no inicializado"));
        return;
    }
    
    DEBUG_PRINTLN(F("📡 === CONFIGURACIÓN RADIO ==="));
    DEBUG_PRINTF("Frecuencia: %.1f MHz\n", LORA_FREQUENCY);
    DEBUG_PRINTF("Bandwidth: %.1f kHz\n", LORA_BANDWIDTH);
    DEBUG_PRINTF("Spreading Factor: %d\n", LORA_SPREADING_FACTOR);
    DEBUG_PRINTF("Coding Rate: 4/%d\n", LORA_CODING_RATE);
    DEBUG_PRINTF("Potencia: %d dBm\n", LORA_OUTPUT_POWER);
    DEBUG_PRINTF("Límite corriente: %d mA\n", LORA_CURRENT_LIMIT);
    DEBUG_PRINTF("Sync Word: 0x%02X\n", LORA_SYNC_WORD);
    DEBUG_PRINTLN(F("================================"));
}

// ============================================================================
// UTILIDADES
// ============================================================================
String LoRaController::getStatusString() {
    if (!initialized) {
        return "Radio: NOT_INIT";
    }
    
    return "Radio: OK, TX:" + String(packet_counter) + 
           ", RSSI:" + String(last_rssi) + 
           ", SNR:" + String(last_snr, 1);
}

void LoRaController::resetCounters() {
    packet_counter = 0;
    last_transmission = 0;
    DEBUG_PRINTLN(F("📡 Contadores del radio reseteados"));
}