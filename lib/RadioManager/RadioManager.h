/**
 * ============================================================================
 * COLLAR GEOFENCING - MÓDULO RADIO
 * ============================================================================
 * 
 * Gestión del radio SX1262 para comunicación LoRa/LoRaWAN
 * 
 * @file RadioManager.h
 * @version 3.0
 */

#ifndef RADIO_MANAGER_H
#define RADIO_MANAGER_H

#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include "config.h"

class RadioManager {
private:
    static SX1262* radio;
    static bool initialized;
    static bool lorawan_mode;
    static uint16_t packet_counter;
    static uint32_t last_transmission;
    static int last_rssi;
    static float last_snr;
    
    // Configuración interna
    static bool setupSPI();
    static bool testConnection();

public:
    // *** INICIALIZACIÓN ***
    static bool init(bool enable_lorawan = false);
    static bool isInitialized();
    static void deinit();
    
    // *** CONFIGURACIÓN BÁSICA ***
    static bool setFrequency(float frequency);
    static bool setBandwidth(float bandwidth);
    static bool setSpreadingFactor(uint8_t sf);
    static bool setCodingRate(uint8_t cr);
    static bool setOutputPower(int8_t power);
    static bool setCurrentLimit(uint8_t current);
    
    // *** TRANSMISIÓN ***
    static bool transmit(const String& data);
    static bool transmit(const uint8_t* data, size_t length);
    static bool transmitPacket(const Position& pos, const BatteryStatus& battery, AlertLevel alert_level);
    
    // *** RECEPCIÓN ***
    static bool receive(String& data);
    static bool receive(uint8_t* data, size_t& length);
    static bool available();
    
    // *** LORAWAN ***
    static bool initLoRaWAN();
    static bool joinOTAA(const String& dev_eui, const String& app_eui, const String& app_key);
    static bool joinABP(const String& dev_addr, const String& nwk_s_key, const String& app_s_key);
    static bool sendLoRaWAN(const uint8_t* data, size_t length, uint8_t port = 1);
    static bool isJoined();
    
    // *** INFORMACIÓN ***
    static uint16_t getPacketCounter();
    static int getLastRSSI();
    static float getLastSNR();
    static uint32_t getLastTransmissionTime();
    static String getRadioInfo();
    
    // *** UTILIDADES ***
    static void reset();
    static void sleep();
    static void wakeup();
    static bool testRadio();
    static void printStatus();
    
    // *** CALLBACKS ***
    static void onReceive(void (*callback)(void));
    static void onTransmit(void (*callback)(void));
};

#endif // RADIO_MANAGER_H