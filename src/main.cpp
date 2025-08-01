/*
 * ============================================================================
 * COLLAR LORAWAN COMPLETO - LED DEBUGGING ONLY
 * Sistema 100% funcional sin depender de Serial
 * ============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <SSD1306Wire.h>

// Incluir todos los managers (sabemos que funcionan)
#include "core/Types.h"
#include "config/pins.h"
#include "config/constants.h"
#include "hardware/BuzzerManager.h"
#include "hardware/PowerManager.h"
#include "hardware/DisplayManager.h"
#include "hardware/GPSManager.h"
#include "hardware/RadioManager.h"

// LED DEBUG PATTERNS
void ledSignal(int pattern) {
    switch(pattern) {
        case 1: // Sistema iniciando
            for(int i=0; i<3; i++) { digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100); }
            break;
        case 2: // Manager inicializado OK
            for(int i=0; i<2; i++) { digitalWrite(LED_PIN, HIGH); delay(200); digitalWrite(LED_PIN, LOW); delay(200); }
            break;
        case 3: // Error en manager
            for(int i=0; i<5; i++) { digitalWrite(LED_PIN, HIGH); delay(50); digitalWrite(LED_PIN, LOW); delay(50); }
            break;
        case 4: // Setup completo
            for(int i=0; i<6; i++) { digitalWrite(LED_PIN, HIGH); delay(150); digitalWrite(LED_PIN, LOW); delay(150); }
            break;
        case 5: // GPS Fix obtenido
            digitalWrite(LED_PIN, HIGH); delay(500); digitalWrite(LED_PIN, LOW);
            break;
        case 6: // LoRaWAN packet enviado
            digitalWrite(LED_PIN, HIGH); delay(50); digitalWrite(LED_PIN, LOW); delay(50);
            digitalWrite(LED_PIN, HIGH); delay(50); digitalWrite(LED_PIN, LOW);
            break;
        case 9: // Heartbeat normal
            digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
            break;
    }
}

// Variables globales
BuzzerManager buzzerManager(BUZZER_PIN);
PowerManager powerManager(VBAT_PIN);
DisplayManager displayManager;
GPSManager gpsManager;
RadioManager radioManager;

uint32_t lastGPSCheck = 0;
uint32_t lastBatteryCheck = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastLoRaTransmit = 0;
uint32_t lastHeartbeat = 0;
uint16_t packetCounter = 0;
bool systemReady = false;
bool loraJoined = false;

// Configuración LoRaWAN ABP (ejemplo)
uint8_t devAddr[4] = {0x01, 0x02, 0x03, 0x04};
uint8_t nwkSKey[16] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 
                       0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
uint8_t appSKey[16] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 
                       0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};

void setup() {
    // CRÍTICO: Activar VEXT inmediatamente
    pinMode(VEXT_ENABLE, OUTPUT);
    digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE);
    delay(200);
    
    // LED para debugging
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Señal: Sistema iniciando
    ledSignal(1);
    delay(1000);
    
    // Inicializar I2C
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(I2C_FREQUENCY);
    
    // 1. BuzzerManager
    if (buzzerManager.init() == Result::SUCCESS) {
        ledSignal(2); // OK
        buzzerManager.playTone(1000, 200, 80);
        delay(300);
    } else {
        ledSignal(3); // Error
    }
    delay(500);
    
    // 2. PowerManager
    if (powerManager.init() == Result::SUCCESS) {
        ledSignal(2); // OK
        powerManager.readBattery();
    } else {
        ledSignal(3); // Error
    }
    delay(500);
    
    // 3. DisplayManager
    if (displayManager.init() == Result::SUCCESS) {
        ledSignal(2); // OK
        displayManager.showSplashScreen();
        delay(2000);
    } else {
        ledSignal(3); // Error
    }
    delay(500);
    
    // 4. GPSManager
    if (gpsManager.init() == Result::SUCCESS) {
        ledSignal(2); // OK
    } else {
        ledSignal(3); // Error
    }
    delay(500);
    
    // 5. RadioManager
    if (radioManager.init() == Result::SUCCESS) {
        ledSignal(2); // OK
        
        // Configurar LoRa
        if (radioManager.setupLoRa(917.0, 125.0, 9, 7, 20) == Result::SUCCESS) {
            ledSignal(2); // LoRa OK
            
            // Configurar LoRaWAN
            if (radioManager.setupLoRaWAN() == Result::SUCCESS) {
                ledSignal(2); // LoRaWAN OK
                
                // Join ABP
                if (radioManager.joinABP(devAddr, nwkSKey, appSKey) == Result::SUCCESS) {
                    loraJoined = true;
                    buzzerManager.playSuccessTone();
                    // 6 parpadeos = LoRaWAN Join exitoso
                    for(int i=0; i<6; i++) {
                        digitalWrite(LED_PIN, HIGH); delay(100);
                        digitalWrite(LED_PIN, LOW); delay(100);
                    }
                }
            }
        }
    } else {
        ledSignal(3); // Error
    }
    
    // Sistema listo
    systemReady = true;
    ledSignal(4); // Setup completo
    delay(2000);
    
    // Inicializar timers
    lastGPSCheck = millis();
    lastBatteryCheck = millis();
    lastDisplayUpdate = millis();
    lastLoRaTransmit = millis();
    lastHeartbeat = millis();
}

void loop() {
    uint32_t currentTime = millis();
    
    // Actualizar GPS
    gpsManager.update();
    radioManager.processDownlinks();
    
    // Check GPS cada 10 segundos
    if (currentTime - lastGPSCheck > 10000) {
        Position gpsPos = gpsManager.getPosition();
        bool hasGPSFix = gpsManager.hasValidFix();
        
        if (hasGPSFix) {
            ledSignal(5); // GPS fix signal
        }
        
        lastGPSCheck = currentTime;
    }
    
    // Check batería cada 30 segundos
    if (currentTime - lastBatteryCheck > 30000) {
        powerManager.readBattery();
        BatteryStatus battery = powerManager.getBatteryStatus();
        
        // Si batería muy baja, parpadeos rápidos de alerta
        if (battery.percentage < 20) {
            for(int i=0; i<3; i++) {
                digitalWrite(LED_PIN, HIGH); delay(50);
                digitalWrite(LED_PIN, LOW); delay(50);
            }
        }
        
        lastBatteryCheck = currentTime;
    }
    
    // Actualizar display cada 15 segundos
    if (currentTime - lastDisplayUpdate > 15000) {
        if (displayManager.isInitialized()) {
            BatteryStatus battery = powerManager.getBatteryStatus();
            Position gpsPos = gpsManager.getPosition();
            bool hasGPSFix = gpsManager.hasValidFix();
            
            SystemStatus status;
            status.buzzerInitialized = true;
            status.displayInitialized = displayManager.isInitialized();
            status.gpsInitialized = gpsManager.isInitialized();
            status.radioInitialized = radioManager.isInitialized();
            status.uptime = currentTime / 1000;
            status.freeHeap = ESP.getFreeHeap();
            status.currentState = (hasGPSFix ? 2 : 0) + (loraJoined ? 1 : 0);
            
            displayManager.showMainScreen(status, gpsPos, battery, AlertLevel::SAFE);
        }
        
        lastDisplayUpdate = currentTime;
    }
    
    // Transmitir por LoRaWAN cada 60 segundos
    if (loraJoined && currentTime - lastLoRaTransmit > 60000) {
        Position gpsPos = gpsManager.getPosition();
        bool hasGPSFix = gpsManager.hasValidFix();
        
        if (hasGPSFix) {
            Result txResult = radioManager.sendPosition(gpsPos, AlertLevel::SAFE);
            if (txResult == Result::SUCCESS) {
                packetCounter++;
                ledSignal(6); // Packet sent signal
                buzzerManager.playTone(1200, 50, 30); // Confirmación
            } else {
                ledSignal(3); // Error
            }
        }
        
        lastLoRaTransmit = currentTime;
    }
    
    // Heartbeat cada 20 segundos
    if (currentTime - lastHeartbeat > 20000) {
        ledSignal(9); // Heartbeat
        lastHeartbeat = currentTime;
    }
    
    delay(100);
}
