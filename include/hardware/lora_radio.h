/*
 * ============================================================================
 * MÓDULO RADIO LORA - Comunicaciones SX1262
 * ============================================================================
 * 
 * Sistema de comunicaciones LoRa para transmisión de datos de geofencing.
 * Preparado para migración a LoRaWAN completo.
 * ============================================================================
 */

#ifndef LORA_RADIO_H
#define LORA_RADIO_H

#include "config.h"
#include <RadioLib.h>
#include <SPI.h>

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================
typedef struct {
    uint16_t packet_id;
    double latitude;
    double longitude;
    float altitude;
    uint8_t alert_level;
    float battery_voltage;
    uint8_t satellites;
    uint32_t timestamp;
    uint8_t checksum;
} LoRaPacket_t;

typedef struct {
    uint8_t command;
    uint8_t parameter;
    uint32_t timestamp;
    uint8_t checksum;
} LoRaDownlink_t;

// Comandos de downlink
enum LoRaCommands {
    CMD_PING = 0x01,
    CMD_SET_INTERVAL = 0x02,
    CMD_TRIGGER_ALERT = 0x03,
    CMD_SET_GEOFENCE = 0x04,
    CMD_REBOOT = 0x05,
    CMD_LOW_POWER = 0x06
};

// ============================================================================
// CLASE RADIO CONTROLLER
// ============================================================================
class LoRaController {
private:
    SX1262* radio;
    bool initialized;
    uint16_t packet_counter;
    uint32_t last_transmission;
    uint32_t last_receive_check;
    int last_rssi;
    float last_snr;
    
    // Configuración y inicialización
    bool setupSPI();
    bool configureRadio();
    
    // Manejo de paquetes
    uint8_t calculateChecksum(const uint8_t* data, size_t length);
    bool validatePacket(const LoRaPacket_t* packet);
    bool validateDownlink(const LoRaDownlink_t* downlink);
    
    // Procesamiento de downlinks
    void processDownlink(const LoRaDownlink_t* downlink);
    
public:
    LoRaController();
    ~LoRaController();
    
    // Inicialización y control
    bool begin();
    void end();
    bool isInitialized() const { return initialized; }
    
    // Transmisión
    bool sendPositionPacket(const Position_t& pos, uint8_t alert_level, float battery_voltage);
    bool sendHeartbeat();
    bool sendAlert(uint8_t alert_level, float distance);
    bool sendCustomPacket(const char* payload);
    
    // Recepción
    void checkDownlinks();
    bool isDownlinkAvailable();
    
    // Estadísticas
    uint16_t getPacketCounter() const { return packet_counter; }
    int getLastRSSI() const { return last_rssi; }
    float getLastSNR() const { return last_snr; }
    uint32_t getLastTransmission() const { return last_transmission; }
    
    // Configuración
    bool setFrequency(float frequency);
    bool setBandwidth(float bandwidth);
    bool setSpreadingFactor(uint8_t sf);
    bool setCodingRate(uint8_t cr);
    bool setOutputPower(int8_t power);
    
    // Test y diagnóstico
    void runSelfTest();
    bool transmitTest();
    void printConfiguration();
    
    // Utilidades
    String getStatusString();
    void resetCounters();
};

// ============================================================================
// INSTANCIA GLOBAL
// ============================================================================
extern LoRaController Radio;

#endif // LORA_RADIO_H