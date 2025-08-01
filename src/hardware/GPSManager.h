#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>
#include "config/pins.h"
#include "config/constants.h"
#include "core/Types.h"
#include "core/Logger.h"

/*
 * ============================================================================
 * GPS MANAGER - GESTIÓN GPS REAL POR UART
 * ============================================================================
 */

class GPSManager {
public:
    GPSManager(uint8_t rxPin = GPS_RX_PIN, uint8_t txPin = GPS_TX_PIN, uint32_t baudRate = GPS_BAUD);
    
    // Inicialización
    Result init();
    bool isInitialized() const;
    
    // Lectura de datos GPS
    void update();
    Position getPosition() const;
    bool hasValidFix() const;
    bool hasNewData() const;
    
    // Información del GPS
    uint8_t getSatelliteCount() const;
    float getHDOP() const;
    float getAltitude() const;
    float getSpeed() const;           // km/h
    float getCourse() const;          // grados (0-360)
    uint32_t getLastUpdateTime() const;
    
    // Configuración
    void setUpdateRate(uint16_t rateMs = 1000);  // 1Hz por defecto
    void setMinSatellites(uint8_t minSats = GPS_MIN_SATELLITES);
    void setAccuracyThreshold(float threshold = GPS_ACCURACY_THRESHOLD);
    
    // Estadísticas
    uint32_t getTotalSentences() const;
    uint32_t getValidSentences() const;
    uint32_t getErrorCount() const;
    float getFixRate() const;         // Porcentaje de tiempo con fix
    
    // Callbacks para eventos
    typedef void (*PositionCallback)(const Position& position);
    typedef void (*FixCallback)(bool hasFix, uint8_t satellites);
    
    void setPositionCallback(PositionCallback callback);
    void setFixCallback(FixCallback callback);
    
    // Estados del GPS
    enum GPSState {
        GPS_IDLE,
        GPS_SEARCHING,
        GPS_FIX_2D,
        GPS_FIX_3D,
        GPS_ERROR
    };
    
    GPSState getState() const;
    const char* getStateString() const;
    
    // Gestión de energía
    void enableLowPowerMode();
    void disableLowPowerMode();
    bool isLowPowerMode() const;
    
private:
    // Hardware
    HardwareSerial* gpsSerial;
    uint8_t rxPin, txPin;
    uint32_t baudRate;
    bool initialized;
    
    // Estado actual
    Position currentPosition;
    GPSState currentState;
    bool hasValidData;
    bool newDataAvailable;
    bool lowPowerMode;
    
    // Configuración
    uint16_t updateRate;
    uint8_t minSatellites;
    float accuracyThreshold;
    uint32_t lastUpdate;
    
    // Estadísticas
    uint32_t totalSentences;
    uint32_t validSentences;
    uint32_t errorCount;
    uint32_t fixStartTime;
    uint32_t totalFixTime;
    
    // Callbacks
    PositionCallback positionCallback;
    FixCallback fixCallback;
    
    // Buffer para NMEA
    static const size_t NMEA_BUFFER_SIZE = 128;
    char nmeaBuffer[NMEA_BUFFER_SIZE];
    size_t bufferIndex;
    
    // Variables temporales para parsing
    struct NMEAData {
        double latitude;
        double longitude;
        float altitude;
        float speed;
        float course;
        uint8_t satellites;
        float hdop;
        bool fixValid;
        uint8_t fixQuality;
        char timestamp[16];
        char date[16];
    } nmeaData;
    
    // Métodos privados
    void readSerialData();
    bool parseNMEASentence(const char* sentence);
    bool parseGGA(const char* sentence);
    bool parseRMC(const char* sentence);
    bool parseGSA(const char* sentence);
    bool parseGSV(const char* sentence);
    
    // Utilidades de parsing
    double parseCoordinate(const char* coord, const char* direction);
    float parseFloat(const char* str);
    int parseInt(const char* str);
    bool getField(const char* sentence, int fieldNumber, char* buffer, size_t bufferSize);
    
    // Validación y filtrado
    bool isValidPosition(double lat, double lng) const;
    bool passesAccuracyFilter() const;
    void updateStatistics();
    void updateState();
    
    // Callbacks internos
    void triggerPositionCallback();
    void triggerFixCallback(bool newFixStatus);
    
    // Debug y logging
    void logGPSInfo() const;
    void logNMEASentence(const char* sentence) const;
};
