/**
 * GPSManager simplificado basado en el test exitoso
 * Versión corregida que funciona con NEO-M8N
 */

#include "GPSManager.h"
#include "../core/Logger.h"
#include <HardwareSerial.h>

// ============================================================================
// CONSTRUCTOR E INICIALIZACIÓN
// ============================================================================

GPSManager::GPSManager(uint8_t rxPin, uint8_t txPin, uint32_t baudRate) :
    gpsSerial(&Serial1),
    rxPin(rxPin), txPin(txPin), baudRate(baudRate),
    initialized(false),
    currentState(GPS_IDLE),
    hasValidData(false),
    newDataAvailable(false),
    lowPowerMode(false),
    updateRate(1000),
    minSatellites(3),
    accuracyThreshold(50.0f),
    lastUpdate(0),
    totalSentences(0),
    validSentences(0),
    errorCount(0),
    fixStartTime(0),
    totalFixTime(0),
    positionCallback(nullptr),
    fixCallback(nullptr),
    bufferIndex(0)
{
    memset(nmeaBuffer, 0, sizeof(nmeaBuffer));
    memset(&nmeaData, 0, sizeof(nmeaData));
}

Result GPSManager::init() {
    if (initialized) {
        return Result::SUCCESS;
    }
    
    LOG_I("🛰️ Inicializando GPS Manager...");
    LOG_I("   Pines: RX=%d, TX=%d", rxPin, txPin);
    LOG_I("   Baudrate: %lu", baudRate);
    
    // Configurar UART para GPS
    gpsSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
    
    // Esperar un momento para estabilización
    delay(100);
    
    // Verificar si hay datos disponibles
    uint32_t startTime = millis();
    bool dataReceived = false;
    
    while (millis() - startTime < 3000) { // Esperar hasta 3 segundos
        if (gpsSerial->available()) {
            dataReceived = true;
            break;
        }
        delay(100);
    }
    
    if (!dataReceived) {
        LOG_W("🛰️ No se detectan datos GPS - verificar conexiones");
        LOG_W("   TX del GPS (blanco) -> GPIO %d (RX del ESP32)", rxPin);
        LOG_W("   RX del GPS (verde) -> GPIO %d (TX del ESP32)", txPin);
        LOG_W("   El LED azul del GPS debe parpadear cuando encuentra satélites");
    } else {
        LOG_I("🛰️ Datos GPS detectados! Esperando fix...");
    }
    
    currentState = GPS_SEARCHING;
    initialized = true;
    
    LOG_INIT("GPS Manager", true);
    return Result::SUCCESS;
}

bool GPSManager::isInitialized() const {
    return initialized;
}

// ============================================================================
// LECTURA Y PROCESAMIENTO DE DATOS
// ============================================================================

void GPSManager::update() {
    if (!initialized) return;
    
    readSerialData();
    
    // Actualizar estado cada segundo
    uint32_t currentTime = millis();
    if (currentTime - lastUpdate > updateRate) {
        updateState();
        updateStatistics();
        lastUpdate = currentTime;
        
        // Log periódico cada 30 segundos
        static uint32_t lastLog = 0;
        if (currentTime - lastLog > 30000) {
            logGPSInfo();
            lastLog = currentTime;
        }
    }
}

Position GPSManager::getPosition() const {
    return currentPosition;
}

bool GPSManager::hasValidFix() const {
    return hasValidData && currentPosition.valid;
}

bool GPSManager::hasNewData() const {
    return newDataAvailable;
}

// ============================================================================
// INFORMACIÓN DEL GPS
// ============================================================================

uint8_t GPSManager::getSatelliteCount() const {
    return nmeaData.satellites;
}

float GPSManager::getHDOP() const {
    return nmeaData.hdop;
}

float GPSManager::getAltitude() const {
    return currentPosition.altitude;
}

float GPSManager::getSpeed() const {
    return nmeaData.speed;
}

float GPSManager::getCourse() const {
    return nmeaData.course;
}

uint32_t GPSManager::getLastUpdateTime() const {
    return currentPosition.timestamp;
}

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

void GPSManager::setUpdateRate(uint16_t rateMs) {
    updateRate = max(rateMs, (uint16_t)100);
}

void GPSManager::setMinSatellites(uint8_t minSats) {
    minSatellites = max(minSats, (uint8_t)3);
}

void GPSManager::setAccuracyThreshold(float threshold) {
    accuracyThreshold = max(threshold, 1.0f);
}

// ============================================================================
// ESTADÍSTICAS
// ============================================================================

uint32_t GPSManager::getTotalSentences() const {
    return totalSentences;
}

uint32_t GPSManager::getValidSentences() const {
    return validSentences;
}

uint32_t GPSManager::getErrorCount() const {
    return errorCount;
}

float GPSManager::getFixRate() const {
    uint32_t uptime = millis();
    if (uptime == 0) return 0.0f;
    return (float)totalFixTime / uptime * 100.0f;
}

// ============================================================================
// CALLBACKS
// ============================================================================

void GPSManager::setPositionCallback(PositionCallback callback) {
    positionCallback = callback;
}

void GPSManager::setFixCallback(FixCallback callback) {
    fixCallback = callback;
}

// ============================================================================
// ESTADO DEL GPS
// ============================================================================

GPSManager::GPSState GPSManager::getState() const {
    return currentState;
}

const char* GPSManager::getStateString() const {
    switch (currentState) {
        case GPS_IDLE:      return "IDLE";
        case GPS_SEARCHING: return "SEARCHING";
        case GPS_FIX_2D:    return "FIX_2D";
        case GPS_FIX_3D:    return "FIX_3D";
        case GPS_ERROR:     return "ERROR";
        default:            return "UNKNOWN";
    }
}

// ============================================================================
// GESTIÓN DE ENERGÍA
// ============================================================================

void GPSManager::enableLowPowerMode() {
    lowPowerMode = true;
    updateRate = 5000;
    LOG_I("🛰️ GPS en modo bajo consumo");
}

void GPSManager::disableLowPowerMode() {
    lowPowerMode = false;
    updateRate = 1000;
    LOG_I("🛰️ GPS en modo normal");
}

bool GPSManager::isLowPowerMode() const {
    return lowPowerMode;
}

// ============================================================================
// MÉTODOS PRIVADOS - LECTURA SERIAL
// ============================================================================

void GPSManager::readSerialData() {
    static bool firstData = true;
    
    while (gpsSerial->available()) {
        char c = gpsSerial->read();
        
        if (firstData) {
            LOG_I("🛰️ Recibiendo datos del GPS!");
            firstData = false;
        }
        
        if (c == '$') {
            // Inicio de nueva sentencia NMEA
            bufferIndex = 0;
            nmeaBuffer[bufferIndex++] = c;
        } else if (c == '\n' || c == '\r') {
            // Fin de sentencia
            if (bufferIndex > 0 && bufferIndex < NMEA_BUFFER_SIZE) {
                nmeaBuffer[bufferIndex] = '\0';
                totalSentences++;
                
                if (parseNMEASentence(nmeaBuffer)) {
                    validSentences++;
                } else {
                    errorCount++;
                }
                
                bufferIndex = 0;
            }
        } else if (bufferIndex < NMEA_BUFFER_SIZE - 1) {
            nmeaBuffer[bufferIndex++] = c;
        } else {
            // Buffer overflow
            bufferIndex = 0;
            errorCount++;
        }
    }
}

// ============================================================================
// PARSING NMEA - Simplificado basado en el test exitoso
// ============================================================================

bool GPSManager::parseNMEASentence(const char* sentence) {
    if (!sentence || strlen(sentence) < 6) return false;
    
    // Solo procesamos GGA para simplificar
    if (strstr(sentence, "GGA")) {
        return parseGGA(sentence);
    } else if (strstr(sentence, "GSV")) {
        // Satélites visibles
        return true; // Solo para indicar que recibimos datos
    }
    
    return false;
}

bool GPSManager::parseGGA(const char* sentence) {
    // Buscar comas para separar campos
    int commaIndex[15];
    int commaCount = 0;
    
    for (int i = 0; i < strlen(sentence) && commaCount < 15; i++) {
        if (sentence[i] == ',') {
            commaIndex[commaCount++] = i;
        }
    }
    
    if (commaCount < 10) return false; // No hay suficientes campos
    
    // Campo 6: Calidad del fix (0 = sin fix, 1 = GPS fix, 2 = DGPS fix)
    char fixQuality = sentence[commaIndex[5] + 1];
    nmeaData.fixValid = (fixQuality > '0');
    
    // Campo 7: Número de satélites
    if (commaIndex[6] + 1 < commaIndex[7]) {
        char satStr[4] = {0};
        int len = commaIndex[7] - commaIndex[6] - 1;
        if (len > 0 && len < 4) {
            strncpy(satStr, &sentence[commaIndex[6] + 1], len);
            nmeaData.satellites = atoi(satStr);
        }
    }
    
    if (!nmeaData.fixValid) {
        // Reportar satélites aunque no haya fix
        if (nmeaData.satellites > 0) {
            static uint8_t lastSats = 0;
            if (nmeaData.satellites != lastSats) {
                LOG_I("🛰️ Satélites visibles: %d (sin fix aún)", nmeaData.satellites);
                lastSats = nmeaData.satellites;
            }
        }
        return true;
    }
    
    // Campo 2: Latitud (ddmm.mmmm)
    if (commaIndex[1] + 1 < commaIndex[2]) {
        char latStr[12] = {0};
        int len = commaIndex[2] - commaIndex[1] - 1;
        if (len > 0 && len < 12) {
            strncpy(latStr, &sentence[commaIndex[1] + 1], len);
            double latRaw = atof(latStr);
            
            // Convertir de NMEA a grados decimales
            int latDeg = (int)(latRaw / 100);
            double latMin = latRaw - (latDeg * 100);
            nmeaData.latitude = latDeg + (latMin / 60.0);
            
            // Campo 3: N/S
            if (sentence[commaIndex[2] + 1] == 'S') {
                nmeaData.latitude = -nmeaData.latitude;
            }
        }
    }
    
    // Campo 4: Longitud (dddmm.mmmm)
    if (commaIndex[3] + 1 < commaIndex[4]) {
        char lngStr[13] = {0};
        int len = commaIndex[4] - commaIndex[3] - 1;
        if (len > 0 && len < 13) {
            strncpy(lngStr, &sentence[commaIndex[3] + 1], len);
            double lngRaw = atof(lngStr);
            
            // Convertir de NMEA a grados decimales
            int lngDeg = (int)(lngRaw / 100);
            double lngMin = lngRaw - (lngDeg * 100);
            nmeaData.longitude = lngDeg + (lngMin / 60.0);
            
            // Campo 5: E/W
            if (sentence[commaIndex[4] + 1] == 'W') {
                nmeaData.longitude = -nmeaData.longitude;
            }
        }
    }
    
    // Campo 8: HDOP
    if (commaIndex[7] + 1 < commaIndex[8]) {
        char hdopStr[8] = {0};
        int len = commaIndex[8] - commaIndex[7] - 1;
        if (len > 0 && len < 8) {
            strncpy(hdopStr, &sentence[commaIndex[7] + 1], len);
            nmeaData.hdop = atof(hdopStr);
        }
    }
    
    // Campo 9: Altitud
    if (commaIndex[8] + 1 < commaIndex[9]) {
        char altStr[10] = {0};
        int len = commaIndex[9] - commaIndex[8] - 1;
        if (len > 0 && len < 10) {
            strncpy(altStr, &sentence[commaIndex[8] + 1], len);
            nmeaData.altitude = atof(altStr);
        }
    }
    
    // Actualizar posición si tenemos coordenadas válidas
    if (nmeaData.latitude != 0.0 && nmeaData.longitude != 0.0) {
        currentPosition.latitude = nmeaData.latitude;
        currentPosition.longitude = nmeaData.longitude;
        currentPosition.altitude = nmeaData.altitude;
        currentPosition.satellites = nmeaData.satellites;
        currentPosition.accuracy = nmeaData.hdop * 3.0f;
        currentPosition.timestamp = millis();
        currentPosition.valid = true;
        
        hasValidData = true;
        newDataAvailable = true;
        
        // Log exitoso como en el test
        LOG_I("\n=== GPS FIX VÁLIDO ===");
        LOG_I("Latitud: %.6f", nmeaData.latitude);
        LOG_I("Longitud: %.6f", nmeaData.longitude);
        LOG_I("Satélites: %d", nmeaData.satellites);
        LOG_I("===================\n");
        
        triggerPositionCallback();
    }
    
    return true;
}

bool GPSManager::parseRMC(const char* sentence) {
    // Simplificado - no necesario para posición básica
    return true;
}

bool GPSManager::parseGSA(const char* sentence) {
    // Simplificado - no necesario para posición básica
    return true;
}

// ============================================================================
// UTILIDADES DE PARSING
// ============================================================================

double GPSManager::parseCoordinate(const char* coord, const char* direction) {
    return atof(coord);
}

float GPSManager::parseFloat(const char* str) {
    if (!str || strlen(str) == 0) return 0.0f;
    return atof(str);
}

int GPSManager::parseInt(const char* str) {
    if (!str || strlen(str) == 0) return 0;
    return atoi(str);
}

bool GPSManager::getField(const char* sentence, int fieldNumber, char* buffer, size_t bufferSize) {
    // Función simplificada - no se usa en esta versión
    return false;
}

// ============================================================================
// VALIDACIÓN Y FILTRADO
// ============================================================================

bool GPSManager::isValidPosition(double lat, double lng) const {
    return lat >= -90.0 && lat <= 90.0 && 
           lng >= -180.0 && lng <= 180.0 &&
           lat != 0.0 && lng != 0.0;
}

bool GPSManager::passesAccuracyFilter() const {
    // Simplificado: solo verificar satélites mínimos
    return nmeaData.satellites >= 3;
}

void GPSManager::updateStatistics() {
    if (hasValidData) {
        if (fixStartTime == 0) {
            fixStartTime = millis();
        }
        totalFixTime += updateRate;
    } else {
        fixStartTime = 0;
    }
}

void GPSManager::updateState() {
    GPSState oldState = currentState;
    
    if (!hasValidData) {
        currentState = GPS_SEARCHING;
    } else if (nmeaData.satellites >= 4) {
        currentState = GPS_FIX_3D;
    } else if (nmeaData.satellites >= 3) {
        currentState = GPS_FIX_2D;
    } else {
        currentState = GPS_SEARCHING;
    }
    
    if ((oldState < GPS_FIX_2D) && (currentState >= GPS_FIX_2D)) {
        triggerFixCallback(true);
    } else if ((oldState >= GPS_FIX_2D) && (currentState < GPS_FIX_2D)) {
        triggerFixCallback(false);
    }
}

// ============================================================================
// CALLBACKS INTERNOS
// ============================================================================

void GPSManager::triggerPositionCallback() {
    if (positionCallback && hasValidData) {
        positionCallback(currentPosition);
    }
}

void GPSManager::triggerFixCallback(bool newFixStatus) {
    if (fixCallback) {
        fixCallback(newFixStatus, nmeaData.satellites);
    }
}

// ============================================================================
// DEBUG Y LOGGING
// ============================================================================

void GPSManager::logGPSInfo() const {
    LOG_I("🛰️ GPS Info: %s | Sats: %d | HDOP: %.1f | Fix: %s | Sentences: %lu/%lu", 
          getStateString(),
          nmeaData.satellites,
          nmeaData.hdop,
          hasValidData ? "YES" : "NO",
          validSentences,
          totalSentences);
          
    if (hasValidData) {
        LOG_I("📍 Posición: %.6f, %.6f | Alt: %.1fm | Precisión: %.1fm",
              currentPosition.latitude, currentPosition.longitude,
              currentPosition.altitude, currentPosition.accuracy);
    } else if (nmeaData.satellites > 0) {
        LOG_I("🔍 Buscando fix... Satélites visibles: %d", nmeaData.satellites);
    } else {
        LOG_W("⚠️ Sin satélites visibles - verificar antena y ubicación");
    }
}

void GPSManager::logNMEASentence(const char* sentence) const {
    if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
        LOG_D("🛰️ NMEA: %s", sentence);
    }
}