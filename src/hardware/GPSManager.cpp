#include "GPSManager.h"

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
    minSatellites(GPS_MIN_SATELLITES),
    accuracyThreshold(GPS_ACCURACY_THRESHOLD),
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
        // Continuar de todas formas, puede que el GPS tarde en arrancar
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
    return currentPosition.satellites;
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
    updateRate = max(rateMs, (uint16_t)100); // Mínimo 100ms
}

void GPSManager::setMinSatellites(uint8_t minSats) {
    minSatellites = max(minSats, (uint8_t)3); // Mínimo 3 satélites
}

void GPSManager::setAccuracyThreshold(float threshold) {
    accuracyThreshold = max(threshold, 1.0f); // Mínimo 1 metro
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
    updateRate = 5000; // Actualizar cada 5 segundos en modo bajo consumo
    LOG_I("🛰️ GPS en modo bajo consumo");
}

void GPSManager::disableLowPowerMode() {
    lowPowerMode = false;
    updateRate = 1000; // Volver a 1 segundo normal
    LOG_I("🛰️ GPS en modo normal");
}

bool GPSManager::isLowPowerMode() const {
    return lowPowerMode;
}

// ============================================================================
// MÉTODOS PRIVADOS - LECTURA SERIAL
// ============================================================================

void GPSManager::readSerialData() {
    while (gpsSerial->available()) {
        char c = gpsSerial->read();
        
        if (c == '$') {
            // Inicio de nueva sentencia NMEA
            bufferIndex = 0;
            nmeaBuffer[bufferIndex++] = c;
        } else if (c == '\n' || c == '\r') {
            // Fin de sentencia
            if (bufferIndex > 0) {
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
            // Agregar caracter al buffer
            nmeaBuffer[bufferIndex++] = c;
        } else {
            // Buffer overflow
            bufferIndex = 0;
            errorCount++;
        }
    }
}

// ============================================================================
// PARSING NMEA
// ============================================================================

bool GPSManager::parseNMEASentence(const char* sentence) {
    if (!sentence || strlen(sentence) < 6) {
        return false;
    }
    
    // Identificar tipo de sentencia
    if (strncmp(sentence, "$GPGGA", 6) == 0 || strncmp(sentence, "$GNGGA", 6) == 0) {
        return parseGGA(sentence);
    } else if (strncmp(sentence, "$GPRMC", 6) == 0 || strncmp(sentence, "$GNRMC", 6) == 0) {
        return parseRMC(sentence);
    } else if (strncmp(sentence, "$GPGSA", 6) == 0 || strncmp(sentence, "$GNGSA", 6) == 0) {
        return parseGSA(sentence);
    }
    
    return false; // Sentencia no reconocida
}

bool GPSManager::parseGGA(const char* sentence) {
    // $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
    // Campos: 0=tipo, 1=tiempo, 2=lat, 3=N/S, 4=lng, 5=E/W, 6=calidad, 7=sats, 8=hdop, 9=alt, 10=M, ...
    
    char field[32];
    
    // Calidad del fix (campo 6)
    if (!getField(sentence, 6, field, sizeof(field))) return false;
    nmeaData.fixQuality = parseInt(field);
    nmeaData.fixValid = (nmeaData.fixQuality > 0);
    
    if (!nmeaData.fixValid) return true; // No hay fix, pero parsing OK
    
    // ===== LATITUD (campos 2 y 3) =====
    // Leer valor de latitud en formato NMEA (ddmm.mmmm)
    if (!getField(sentence, 2, field, sizeof(field))) return false;
    double latRaw = parseFloat(field);
    
    // Leer dirección N/S
    if (!getField(sentence, 3, field, sizeof(field))) return false;
    char latDirection = field[0];
    
    // Convertir de NMEA (ddmm.mmmm) a grados decimales
    int latDeg = (int)(latRaw / 100);           // Grados
    double latMin = latRaw - (latDeg * 100);    // Minutos decimales
    nmeaData.latitude = latDeg + (latMin / 60.0); // Grados decimales
    
    // Aplicar dirección (Sur = negativo)
    if (latDirection == 'S') {
        nmeaData.latitude = -nmeaData.latitude;
    }
    
    // ===== LONGITUD (campos 4 y 5) =====
    // Leer valor de longitud en formato NMEA (dddmm.mmmm)
    if (!getField(sentence, 4, field, sizeof(field))) return false;
    double lngRaw = parseFloat(field);
    
    // Leer dirección E/W
    if (!getField(sentence, 5, field, sizeof(field))) return false;
    char lngDirection = field[0];
    
    // Convertir de NMEA (dddmm.mmmm) a grados decimales
    int lngDeg = (int)(lngRaw / 100);           // Grados
    double lngMin = lngRaw - (lngDeg * 100);    // Minutos decimales
    nmeaData.longitude = lngDeg + (lngMin / 60.0); // Grados decimales
    
    // Aplicar dirección (Oeste = negativo)
    if (lngDirection == 'W') {
        nmeaData.longitude = -nmeaData.longitude;
    }
    
    // Satélites (campo 7)
    if (!getField(sentence, 7, field, sizeof(field))) return false;
    nmeaData.satellites = parseInt(field);
    
    // HDOP (campo 8)
    if (!getField(sentence, 8, field, sizeof(field))) return false;
    nmeaData.hdop = parseFloat(field);
    
    // Altitud (campo 9)
    if (!getField(sentence, 9, field, sizeof(field))) return false;
    nmeaData.altitude = parseFloat(field);
    
    // Actualizar posición si es válida
    if (isValidPosition(nmeaData.latitude, nmeaData.longitude) && passesAccuracyFilter()) {
        currentPosition.latitude = nmeaData.latitude;
        currentPosition.longitude = nmeaData.longitude;
        currentPosition.altitude = nmeaData.altitude;
        currentPosition.satellites = nmeaData.satellites;
        currentPosition.accuracy = nmeaData.hdop * 3.0f; // Aproximación de precisión
        currentPosition.timestamp = millis();
        currentPosition.valid = true;
        
        hasValidData = true;
        newDataAvailable = true;
        
        triggerPositionCallback();
        
        // Log para debug
        LOG_D("🛰️ GPS Fix: %.6f°%c, %.6f°%c | Sats: %d | HDOP: %.1f", 
              abs(nmeaData.latitude), (nmeaData.latitude >= 0) ? 'N' : 'S',
              abs(nmeaData.longitude), (nmeaData.longitude >= 0) ? 'E' : 'W',
              nmeaData.satellites, nmeaData.hdop);
    }
    
    return true;
}

bool GPSManager::parseRMC(const char* sentence) {
    // $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
    // Campos: 0=tipo, 1=tiempo, 2=status, 3=lat, 4=N/S, 5=lng, 6=E/W, 7=speed, 8=course, 9=date, ...
    
    char field[32];
    
    // Status (campo 2) - debe ser 'A' para datos válidos
    if (!getField(sentence, 2, field, sizeof(field))) return false;
    if (field[0] != 'A') return true; // No hay datos válidos
    
    // Velocidad (campo 7) - en nudos
    if (getField(sentence, 7, field, sizeof(field))) {
        nmeaData.speed = parseFloat(field) * 1.852f; // Convertir nudos a km/h
    }
    
    // Curso (campo 8) - en grados
    if (getField(sentence, 8, field, sizeof(field))) {
        nmeaData.course = parseFloat(field);
    }
    
    return true;
}

bool GPSManager::parseGSA(const char* sentence) {
    // $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
    // Información sobre modo de fix y precisión
    return true; // Implementación básica
}

// ============================================================================
// UTILIDADES DE PARSING
// ============================================================================

double GPSManager::parseCoordinate(const char* coord, const char* direction) {
    double result = parseFloat(coord);
    return result;
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
    if (!sentence || !buffer || bufferSize == 0) return false;
    
    int currentField = 0;
    const char* start = sentence;
    const char* end;
    
    // Buscar el campo especificado
    while (currentField < fieldNumber) {
        start = strchr(start, ',');
        if (!start) return false;
        start++; // Saltar la coma
        currentField++;
    }
    
    // Encontrar el final del campo
    end = strchr(start, ',');
    if (!end) {
        end = strchr(start, '*'); // Final de sentencia
        if (!end) {
            end = start + strlen(start);
        }
    }
    
    // Copiar el campo al buffer
    size_t length = end - start;
    if (length >= bufferSize) length = bufferSize - 1;
    
    strncpy(buffer, start, length);
    buffer[length] = '\0';
    
    return true;
}

// ============================================================================
// VALIDACIÓN Y FILTRADO
// ============================================================================

bool GPSManager::isValidPosition(double lat, double lng) const {
    return lat >= -90.0 && lat <= 90.0 && lng >= -180.0 && lng <= 180.0 &&
           lat != 0.0 && lng != 0.0; // Evitar coordenadas (0,0)
}

bool GPSManager::passesAccuracyFilter() const {
    return nmeaData.satellites >= minSatellites && 
           nmeaData.hdop > 0 && nmeaData.hdop <= (accuracyThreshold / 3.0f);
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
    } else if (nmeaData.satellites >= 4 && nmeaData.altitude > 0) {
        currentState = GPS_FIX_3D;
    } else if (nmeaData.satellites >= 3) {
        currentState = GPS_FIX_2D;
    } else {
        currentState = GPS_SEARCHING;
    }
    
    // Triggear callback si cambió el estado del fix
    if ((oldState >= GPS_FIX_2D) != (currentState >= GPS_FIX_2D)) {
        triggerFixCallback(currentState >= GPS_FIX_2D);
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
        LOG_GPS(currentPosition.latitude, currentPosition.longitude, true);
    }
}

void GPSManager::logNMEASentence(const char* sentence) const {
    if (LOG_LEVEL >= LOG_LEVEL_DEBUG) {
        LOG_D("🛰️ NMEA: %s", sentence);
    }
}
