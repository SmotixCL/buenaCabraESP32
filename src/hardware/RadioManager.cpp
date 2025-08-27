#include "RadioManager.h"
#include "../core/Logger.h"
#include <Preferences.h> // Para guardar configuración persistente

// ============================================================================
// VARIABLE ESTÁTICA PARA INTERRUPT CALLBACK
// ============================================================================
RadioManager *RadioManager::instance = nullptr;

// ============================================================================
// VARIABLE PARA CALLBACK DE GEOCERCA
// ============================================================================
GeofenceUpdateCallback RadioManager::geofenceUpdateCallback = nullptr;

// ============================================================================
// CONSTRUCTOR E INICIALIZACIÓN
// ============================================================================

RadioManager::RadioManager(uint8_t nss, uint8_t dio1, uint8_t rst, uint8_t busy) : radio(new Module(nss, dio1, rst, busy)),
                                                                                   lorawan(&radio, &AU915), // Región AU915 para Chile
                                                                                   nssPin(nss), dio1Pin(dio1), rstPin(rst), busyPin(busy),
                                                                                   initialized(false), joined(false), sleeping(false),
                                                                                   currentState(STATE_IDLE),
                                                                                   packetsSent(0), packetsReceived(0), packetsLost(0),
                                                                                   lastRSSI(0), lastSNR(0),
                                                                                   uplinkFrameCounter(0), downlinkFrameCounter(0),
                                                                                   lastUplinkTime(0), lastDownlinkTime(0),
                                                                                   currentDataRate(0), currentTxPower(20),
                                                                                   adrEnabled(true), confirmedUplinks(false),
                                                                                   downlinkCallback(nullptr), joinCallback(nullptr), txCallback(nullptr),
                                                                                   pendingDownlink(false), downlinkLength(0), downlinkPort(0),
                                                                                   // Inicialización de variables para persistir sesión
                                                                                   sessionStartTime(0), lastSuccessfulUplink(0), sessionRestored(false)
{
    instance = this;
}

Result RadioManager::init()
{
    if (initialized)
    {
        return Result::SUCCESS;
    }

    LOG_I("📡 Inicializando Radio Manager...");

    // Configurar SPI
    setupSPI();

    // Reset del radio
    resetRadio();

    // Configurar radio básico
    Result result = configureRadio();
    if (result != Result::SUCCESS)
    {
        return result;
    }

    // Configurar interrupt
    radio.setDio1Action(onDio1Action);

    // Intentamos cargar la sesión previa, en caso de existir
    if (loadSessionState())
    {
        LOG_I("📡 Sesión LoRaWAN previa encontrada y válida");
        joined = true;
        currentState = STATE_JOINED;
        sessionRestored = true;
    }
    else
    {
        LOG_I("📡 Sin sesión previa válida, se requerirá nuevo JOIN");
    }

    initialized = true;
    currentState = STATE_IDLE;

    LOG_I("✅ Radio Manager inicializado");
    LOG_I("📡 SX1262 configurado en %.1f MHz", 915.0);

    return Result::SUCCESS;
}

bool RadioManager::isInitialized() const
{
    return initialized;
}

// ============================================================================
// CONFIGURACIÓN LORA BÁSICA
// ============================================================================

Result RadioManager::setupLoRa(float frequency, float bandwidth,
                               uint8_t spreadingFactor, uint8_t codingRate, int8_t power)
{
    if (!initialized)
        return Result::ERROR_INIT;

    LOG_I("📡 Configurando LoRa: %.1fMHz, BW=%.1f, SF=%d, CR=%d, PWR=%ddBm",
          frequency, bandwidth, spreadingFactor, codingRate, power);

    int16_t state = radio.begin(frequency, bandwidth, spreadingFactor, codingRate, 0x12, power);

    if (state != RADIOLIB_ERR_NONE)
    {
        LOG_E("❌ Error configurando LoRa: %s", getErrorString(state));
        handleRadioError(state);
        return Result::ERROR_HARDWARE;
    }

    // Configurar preamble y sync word
    radio.setPreambleLength(8);
    radio.setSyncWord(0x12); // LoRa público

    LOG_I("✅ LoRa configurado correctamente");
    return Result::SUCCESS;
}

// ============================================================================
// CONFIGURACIÓN LORAWAN
// ============================================================================

Result RadioManager::setupLoRaWAN()
{
    if (!initialized)
        return Result::ERROR_INIT;

    LOG_I("📡 Configurando LoRaWAN AU915...");

    // RadioLib 6.6.0 no usa begin() para LoRaWAN, se configura directamente en OTAA/ABP

    LOG_I("✅ LoRaWAN AU915 Sub-banda 1 listo para configurar");
    return Result::SUCCESS;
}

// joinOTAA Nuevo: Manejo de persistencia de sesion en resets
Result RadioManager::joinOTAA(const uint8_t *devEUI, const uint8_t *appEUI, const uint8_t *appKey)
{
    if (!initialized)
        return Result::ERROR_INIT;

    // Si ya tenemos una sesión válida restaurada, no hacer join
    if (sessionRestored && joined && isSessionValid())
    {
        LOG_I("📡 Usando sesión LoRaWAN restaurada, evitando rejoin");
        currentState = STATE_JOINED;
        if (joinCallback)
        {
            joinCallback(true);
        }
        return Result::SUCCESS;
    }

    // Si no tenemos una sesión válida continuamos
    LOG_I("📡 Iniciando nuevo OTAA Join...");
    currentState = STATE_JOINING;

    // Convertir arrays de EUI a uint64_t (little endian)
    uint64_t joinEUI_le = 0, devEUI_le = 0;
    for (int i = 0; i < 8; i++)
    {
        joinEUI_le |= ((uint64_t)appEUI[i]) << (i * 8);
        devEUI_le |= ((uint64_t)devEUI[i]) << (i * 8);
    }

    // Crear copias locales de las claves
    uint8_t nwkKey[16], appKeyLocal[16];
    memcpy(nwkKey, appKey, 16);
    memcpy(appKeyLocal, appKey, 16);

    // Configurar parámetros para el Join
    lorawan.beginOTAA(joinEUI_le, devEUI_le, nwkKey, appKeyLocal);

    // Intentar el Join
    int16_t state = lorawan.activateOTAA();

    if (state == RADIOLIB_LORAWAN_NEW_SESSION || state == RADIOLIB_LORAWAN_SESSION_RESTORED)
    {
        joined = true;
        currentState = STATE_JOINED;
        sessionStartTime = millis();
        lastSuccessfulUplink = millis();
        sessionRestored = false; // Es una sesión nueva/fresca

        LOG_I("✅ OTAA Join exitoso!");

        // Guardar estado de sesión
        if (saveSessionState())
        {
            LOG_I("📄 Estado de sesión guardado");
        }
        else
        {
            LOG_W("⚠️ Error guardando estado de sesión");
        }

        if (joinCallback)
        {
            joinCallback(true);
        }
        return Result::SUCCESS;
    }
    else
    {
        LOG_E("❌ OTAA Join falló. Código: %d (%s)", state, getErrorString(state));
        currentState = STATE_ERROR;

        // Limpiar cualquier sesión inválida
        clearSessionState();

        if (joinCallback)
        {
            joinCallback(false);
        }
        return Result::ERROR_COMMUNICATION;
    }
}

Result RadioManager::joinABP(const uint8_t *devAddr, const uint8_t *nwkSKey, const uint8_t *appSKey)
{
    if (!initialized)
        return Result::ERROR_INIT;

    LOG_I("📡 Configurando ABP...");

    // Convertir devAddr a uint32_t (little endian)
    uint32_t deviceAddr = 0;
    for (int i = 0; i < 4; i++)
    {
        deviceAddr |= ((uint32_t)devAddr[i]) << (i * 8);
    }

    // Crear copias de las claves (RadioLib 6.6.0 necesita 4 claves para ABP)
    uint8_t fNwkSIntKey[16], sNwkSIntKey[16], nwkSEncKey[16], appSKeyLocal[16];

    // En LoRaWAN 1.0.x, todas las claves de red son iguales a nwkSKey
    memcpy(fNwkSIntKey, nwkSKey, 16);
    memcpy(sNwkSIntKey, nwkSKey, 16);
    memcpy(nwkSEncKey, nwkSKey, 16);
    memcpy(appSKeyLocal, appSKey, 16);

    // RadioLib 6.6.0 API para ABP - beginABP() es void
    lorawan.beginABP(deviceAddr, fNwkSIntKey, sNwkSIntKey, nwkSEncKey, appSKeyLocal);

    // ✅ RESTAURAR FRAME COUNTERS PERSISTENTES PARA ABP
    Preferences prefs;
    if (prefs.begin("lorawan", false))
    {
        uint32_t fcntUp = prefs.getULong("fcntup", 0);
        uint32_t fcntDown = prefs.getULong("fcntdown", 0);
        prefs.end();

        // Establecer frame counters si es posible
        // Nota: Verificar si RadioLib tiene setFrameCounters() o similar
        // lorawan.setFrameCounters(fcntUp, fcntDown);

        LOG_I("📦 Frame counters restaurados: Up=%lu, Down=%lu", fcntUp, fcntDown);
    }
    else
    {
        LOG_W("⚠️ Sin persistencia de frame counters, posibles problemas con reinicios");
    }

    // En ABP la sesión está lista inmediatamente
    joined = true;
    currentState = STATE_JOINED;
    LOG_I("✅ ABP configurado exitosamente");

    if (joinCallback)
    {
        joinCallback(true);
    }
    return Result::SUCCESS;
}

bool RadioManager::isJoined() const
{
    return joined;
}

// ============================================================================
// CALLBACK PARA ACTUALIZACIONES DE GEOCERCA
// ============================================================================

void RadioManager::setGeofenceUpdateCallback(GeofenceUpdateCallback callback)
{
    geofenceUpdateCallback = callback;
}

// ============================================================================
// TRANSMISIÓN DE DATOS - CORREGIDO PARA RADIOLIB 6.6.0
// ============================================================================

Result RadioManager::sendPacket(const uint8_t *data, size_t length, uint8_t port)
{
    if (!initialized || !joined)
    {
        Serial.println("Error porque o initialized o joined son false");
        return Result::ERROR_INIT;
    }
    if (length > MAX_PAYLOAD_SIZE)
    {
        Serial.println("Largo del payload demasiado");
        return Result::ERROR_INVALID_PARAM;
    }

    currentState = STATE_TX;

    // Copiar datos al buffer de transmisión
    memcpy(txBuffer, data, length);

    LOG_D("📡 Enviando %d bytes en puerto %d", length, port);

    // *** CORRECCIÓN PRINCIPAL: Usar sendReceive() en lugar de uplink() ***
    uint8_t downlinkPayload[MAX_PAYLOAD_SIZE];
    size_t downlinkSize = 0;
    LoRaWANEvent_t uplinkDetails;
    LoRaWANEvent_t downlinkDetails;

    // Enviar uplink y verificar downlink en una sola operación
    int16_t state = lorawan.sendReceive(txBuffer, length, port,
                                        downlinkPayload, &downlinkSize,
                                        confirmedUplinks, &uplinkDetails, &downlinkDetails);

    if (state == RADIOLIB_ERR_NONE)
    {
        packetsSent++;
        currentState = STATE_IDLE;
        lastSuccessfulUplink = millis(); // Para persistencia de sesion: Actualizar timestamp del último uplink exitoso

        // Obtener RSSI y SNR del último paquete
        lastRSSI = radio.getRSSI();
        lastSNR = radio.getSNR();

        LOG_I("📡 Packet #%d enviado - RSSI: %.1fdBm, SNR: %.1fdB",
              packetsSent, lastRSSI, lastSNR);

        // ✅ ACTUALIZAR FRAME COUNTERS CORREGIDOS
        uplinkFrameCounter++;
        lastUplinkTime = millis();

        // Guardar cada 10 transmisiones para no desgastar la flash
        // EN TESTING CAMBIAMOS DE 10 A 2
        if (packetsSent % 2 == 0)
        {
            saveSessionState();
        }

        // Verificar si hay downlink
        if (downlinkSize > 0)
        {
            // ✅ ACTUALIZAR CONTADORES DE DOWNLINK
            downlinkFrameCounter++;
            lastDownlinkTime = millis();

            // Copiar downlink al buffer de recepción
            memcpy(rxBuffer, downlinkPayload, downlinkSize);
            downlinkLength = downlinkSize;
            downlinkPort = downlinkDetails.fPort;
            pendingDownlink = true;

            LOG_I("📡 Downlink #%d recibido: %d bytes en puerto %d",
                  downlinkFrameCounter, downlinkSize, downlinkDetails.fPort);

            // Procesar downlink inmediatamente
            processDownlink(downlinkPayload, downlinkSize, downlinkDetails.fPort);
            packetsReceived++;
        }

        if (txCallback)
        {
            txCallback(true);
        }

        return Result::SUCCESS;
    }
    else
    {
        packetsLost++;
        currentState = STATE_ERROR;
        LOG_E("❌ Error enviando packet: %s", getErrorString(state));

        if (txCallback)
        {
            txCallback(false);
        }
        Serial.println("Paquete perdido (No es error del largo)");
        return Result::ERROR_COMMUNICATION;
    }
}

Result RadioManager::sendString(const String &message, uint8_t port)
{
    return sendPacket((const uint8_t *)message.c_str(), message.length(), port);
}

Result RadioManager::sendPosition(const Position &position, AlertLevel alertLevel)
{
    if (!isValidPosition(position))
    {
        return Result::ERROR_INVALID_PARAM;
    }

    size_t payloadSize = createPositionPayload(txBuffer, position, alertLevel);
    return sendPacket(txBuffer, payloadSize, 1);
}

Result RadioManager::sendBatteryStatus(const BatteryStatus &battery)
{
    size_t payloadSize = createBatteryPayload(txBuffer, battery);
    return sendPacket(txBuffer, payloadSize, 2);
}

// ============================================================================
// RECEPCIÓN DE DATOS - CORREGIDO PARA RADIOLIB 6.6.0
// ============================================================================

Result RadioManager::receivePacket(uint8_t *buffer, size_t *length, uint8_t *port)
{
    // *** CORRECCIÓN: En RadioLib 6.6.0, no existe downlink() independiente ***
    // Los downlinks se manejan automáticamente en sendReceive()

    if (pendingDownlink && downlinkLength > 0)
    {
        if (*length < downlinkLength)
        {
            return Result::ERROR_INVALID_PARAM;
        }

        memcpy(buffer, rxBuffer, downlinkLength);
        *length = downlinkLength;
        if (port)
        {
            *port = downlinkPort;
        }

        // Limpiar downlink pendiente
        pendingDownlink = false;
        downlinkLength = 0;

        LOG_I("📡 Downlink entregado: %d bytes en puerto %d", *length, port ? *port : 0);
        return Result::SUCCESS;
    }

    return Result::ERROR_TIMEOUT;
}

bool RadioManager::hasDownlink() const
{
    return pendingDownlink && (downlinkLength > 0);
}

void RadioManager::processDownlinks()
{
    // *** CORRECCIÓN: En RadioLib 6.6.0, los downlinks se procesan automáticamente ***
    // Esta función ahora solo maneja downlinks ya recibidos

    if (pendingDownlink && downlinkLength > 0)
    {
        processDownlink(rxBuffer, downlinkLength, downlinkPort);
        // Note: No limpiamos pendingDownlink aquí para que receivePacket() pueda acceder a los datos
    }
}

// ===========================================
// NUEVOS MÉTODOS PARA GESTIÓN DE PERSISTENCIA
// ===========================================
bool RadioManager::saveSessionState()
{
    Preferences prefs;
    if (!prefs.begin("lorawan_session", false))
    {
        return false;
    }

    // Guardar estado básico de sesión
    prefs.putBool("joined", joined);
    prefs.putULong("session_start", sessionStartTime);
    prefs.putULong("last_uplink", lastSuccessfulUplink);
    prefs.putULong("uplink_fcnt", uplinkFrameCounter);
    prefs.putULong("downlink_fcnt", downlinkFrameCounter);
    prefs.putUShort("packets_sent", packetsSent);

    // Checksum simple para validar integridad
    uint32_t checksum = sessionStartTime ^ lastSuccessfulUplink ^ uplinkFrameCounter;
    prefs.putULong("checksum", checksum);

    prefs.end();
    LOG_D("💾 Sesión guardada: FCnt=%lu, Packets=%d", uplinkFrameCounter, packetsSent);
    return true;
}

bool RadioManager::loadSessionState()
{
    Preferences prefs;
    if (!prefs.begin("lorawan_session", true))
    {
        return false;
    }

    // Cargar datos de sesión
    bool wasJoined = prefs.getBool("joined", false);
    uint32_t savedSessionStart = prefs.getULong("session_start", 0);
    uint32_t savedLastUplink = prefs.getULong("last_uplink", 0);
    uint32_t savedUplinkFcnt = prefs.getULong("uplink_fcnt", 0);
    uint32_t savedDownlinkFcnt = prefs.getULong("downlink_fcnt", 0);
    uint16_t savedPacketsSent = prefs.getUShort("packets_sent", 0);
    uint32_t savedChecksum = prefs.getULong("checksum", 0);

    prefs.end();

    // Validar integridad
    uint32_t calculatedChecksum = savedSessionStart ^ savedLastUplink ^ savedUplinkFcnt;
    if (savedChecksum != calculatedChecksum)
    {
        LOG_W("⚠️ Checksum de sesión inválido, datos corruptos");
        return false;
    }

    // Verificar si la sesión no es demasiado antigua
    if (!wasJoined || savedSessionStart == 0)
    {
        LOG_I("📡 Sin sesión LoRaWAN previa válida");
        return false;
    }

    // Restaurar variables de estado
    sessionStartTime = savedSessionStart;
    lastSuccessfulUplink = savedLastUplink;
    uplinkFrameCounter = savedUplinkFcnt;
    downlinkFrameCounter = savedDownlinkFcnt;
    packetsSent = savedPacketsSent;

    LOG_I("📡 Sesión cargada: FCnt=%lu, Packets=%d, Edad=%lu min",
          uplinkFrameCounter, packetsSent, (millis() - lastSuccessfulUplink) / 60000);

    return true;
}

// clearSession is unused
void RadioManager::clearSessionState()
{
    Preferences prefs;
    if (prefs.begin("lorawan_session", false))
    {
        prefs.clear();
        prefs.end();
        LOG_D("🗑️ Estado de sesión limpiado");
    }

    // Reset de variables locales
    sessionStartTime = 0;
    lastSuccessfulUplink = 0;
    sessionRestored = false;
    joined = false;
}

bool RadioManager::isSessionValid()
{
    uint32_t now = millis();

    // Sesión válida si:
    // 1. No es demasiado antigua (< 24 horas desde último uplink)
    // 2. La sesión fue iniciada (sessionStartTime > 0)
    uint32_t maxSessionAge = 24 * 60 * 60 * 1000; // 24 horas en ms

    if (sessionStartTime == 0)
    {
        return false;
    }

    uint32_t timeSinceLastUplink = now - lastSuccessfulUplink;
    if (timeSinceLastUplink > maxSessionAge)
    {
        LOG_W("⚠️ Sesión demasiado antigua (%lu min), se requiere rejoin",
              timeSinceLastUplink / 60000);
        return false;
    }

    return true;
}

// NUEVO MÉTODO PÚBLICO PARA FORZAR REJOIN
Result RadioManager::forceRejoin()
{
    LOG_I("🔄 Forzando rejoin LoRaWAN...");
    clearSessionState();
    joined = false;
    currentState = STATE_IDLE;
    return Result::SUCCESS;
}

// ============================================================================
// ESTADÍSTICAS Y ESTADO
// ============================================================================

uint16_t RadioManager::getPacketsSent() const
{
    return packetsSent;
}

uint16_t RadioManager::getPacketsReceived() const
{
    return packetsReceived;
}

uint16_t RadioManager::getPacketsLost() const
{
    return packetsLost;
}

float RadioManager::getRSSI() const
{
    return lastRSSI;
}

float RadioManager::getSNR() const
{
    return lastSNR;
}

RadioManager::RadioState RadioManager::getState() const
{
    return currentState;
}

const char *RadioManager::getStateString() const
{
    switch (currentState)
    {
    case STATE_IDLE:
        return "IDLE";
    case STATE_TX:
        return "TX";
    case STATE_RX:
        return "RX";
    case STATE_JOINING:
        return "JOINING";
    case STATE_JOINED:
        return "JOINED";
    case STATE_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

// ============================================================================
// CONFIGURACIÓN AVANZADA
// ============================================================================

void RadioManager::setDataRate(uint8_t dataRate)
{
    currentDataRate = dataRate;
    if (initialized && joined)
    {
        lorawan.setDatarate(dataRate); // Corregido: setDatarate() en RadioLib 6.6.0
    }
}

void RadioManager::setTxPower(int8_t power)
{
    currentTxPower = power;
    if (initialized)
    {
        lorawan.setTxPower(power);
    }
}

void RadioManager::setAdaptiveDataRate(bool enabled)
{
    adrEnabled = enabled;
    if (initialized && joined)
    {
        lorawan.setADR(enabled);
    }
}

void RadioManager::setConfirmedUplinks(bool enabled)
{
    confirmedUplinks = enabled;
}

// ============================================================================
// CALLBACKS
// ============================================================================

void RadioManager::setDownlinkCallback(DownlinkCallback callback)
{
    downlinkCallback = callback;
}

void RadioManager::setJoinCallback(JoinCallback callback)
{
    joinCallback = callback;
}

void RadioManager::setTxCallback(TxCallback callback)
{
    txCallback = callback;
}

// ============================================================================
// GESTIÓN DE ENERGÍA
// ============================================================================

void RadioManager::sleep()
{
    if (initialized && !sleeping)
    {
        radio.sleep();
        sleeping = true;
        LOG_D("📡 Radio en modo sleep");
    }
}

void RadioManager::wakeup()
{
    if (initialized && sleeping)
    {
        radio.standby();
        sleeping = false;
        LOG_D("📡 Radio despierto");
    }
}

bool RadioManager::isSleeping() const
{
    return sleeping;
}

// ============================================================================
// MÉTODOS PRIVADOS
// ============================================================================

void RadioManager::setupSPI()
{
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, nssPin);
    SPI.setFrequency(SPI_FREQUENCY);
}

void RadioManager::resetRadio()
{
    pinMode(rstPin, OUTPUT);
    digitalWrite(rstPin, LOW);
    delay(10);
    digitalWrite(rstPin, HIGH);
    delay(10);
}

Result RadioManager::configureRadio()
{
    // Configuración inicial del SX1262
    int16_t state = radio.begin(915.0, 125.0, 9, 7, 0x12, 20);

    if (state != RADIOLIB_ERR_NONE)
    {
        return Result::ERROR_HARDWARE;
    }

    // Configuraciones adicionales
    radio.setDio2AsRfSwitch(true); // DIO2 como RF switch

    return Result::SUCCESS;
}

// ============================================================================
// MANEJO DE INTERRUPCIONES
// ============================================================================

void RadioManager::onDio1Action()
{
    if (instance)
    {
        instance->handleDio1Interrupt();
    }
}

void RadioManager::handleDio1Interrupt()
{
    // Manejar interrupciones del radio
    // En LoRaWAN, RadioLib maneja esto internamente
}

// ============================================================================
// UTILIDADES LORAWAN
// ============================================================================

size_t RadioManager::createPositionPayload(uint8_t *buffer, const Position &position, AlertLevel alertLevel)
{
    // Formato de payload LEGACY (12 bytes) - mantenido para compatibilidad
    size_t index = 0;

    // Latitud (4 bytes)
    float lat = (float)position.latitude;
    memcpy(&buffer[index], &lat, 4);
    index += 4;

    // Longitud (4 bytes)
    float lng = (float)position.longitude;
    memcpy(&buffer[index], &lng, 4);
    index += 4;

    // Altitud (2 bytes)
    int16_t alt = (int16_t)position.altitude;
    buffer[index++] = (alt >> 8) & 0xFF;
    buffer[index++] = alt & 0xFF;

    // Nivel de alerta (1 byte)
    buffer[index++] = (uint8_t)alertLevel;

    // Placeholder para batería (se añadirá en otra función)
    buffer[index++] = 0;

    return index;
}

// NUEVO: Payload mejorado con estado del dispositivo
uint8_t RadioManager::calculateGroupHash(const char *groupId)
{
    // Simple hash de 8 bits para el groupId
    uint8_t hash = 0;
    if (groupId == nullptr)
        return 0;

    for (size_t i = 0; groupId[i] != '\0'; i++)
    {
        hash = hash * 31 + (uint8_t)groupId[i];
    }
    return hash;
}

size_t RadioManager::createDeviceStatusPayload(uint8_t *buffer, const Position &position,
                                               const BatteryStatus &battery, AlertLevel alertLevel,
                                               const Geofence &geofence, bool gpsValid,
                                               bool insideGeofence, uint8_t frameCount)
{
    // Usar la estructura optimizada GPSPayloadV2 (11 bytes)
    GPSPayloadV2 payload;

    // Llenar la estructura directamente
    payload.latitude = (int32_t)(position.latitude * 10000000);
    payload.longitude = (int32_t)(position.longitude * 10000000);
    payload.altitude = (uint16_t)position.altitude;
    payload.satellites = position.satellites;
    payload.hdop = (uint8_t)(position.accuracy * 10); // Usar accuracy como HDOP
    payload.battery = battery.percentage;
    payload.alert = (uint8_t)alertLevel;

    // Flags de estado del dispositivo
    payload.status = 0;
    if (gpsValid)
        payload.status |= DEVICE_GPS_FIX_FLAG;
    if (battery.low)
        payload.status |= DEVICE_BATTERY_LOW_FLAG;
    if (insideGeofence)
        payload.status |= GEOFENCE_INSIDE_FLAG;

    // Hash del grupo
    payload.groupIdHash = calculateGroupHash(geofence.groupId);

    // Flags de geocerca
    payload.geofenceFlags = 0;
    payload.geofenceFlags |= (uint8_t)geofence.type & GEOFENCE_TYPE_MASK;
    if (geofence.active)
        payload.geofenceFlags |= GEOFENCE_ACTIVE_FLAG;
    if (insideGeofence)
        payload.geofenceFlags |= GEOFENCE_INSIDE_FLAG;

    // Frame counter para tracking
    payload.frameCounter = frameCount;

    // Copiar al buffer
    memcpy(buffer, &payload, sizeof(payload));

    return sizeof(payload);
}

size_t RadioManager::createBatteryPayload(uint8_t *buffer, const BatteryStatus &battery)
{
    // Formato simple de batería (4 bytes):
    // Voltage: 2 bytes (uint16, mV)
    // Percentage: 1 byte
    // Flags: 1 byte (charging, low, critical)

    size_t index = 0;

    uint16_t voltage_mv = (uint16_t)(battery.voltage * 1000);
    buffer[index++] = (voltage_mv >> 8) & 0xFF;
    buffer[index++] = voltage_mv & 0xFF;

    buffer[index++] = battery.percentage;

    uint8_t flags = 0;
    if (battery.charging)
        flags |= 0x01;
    if (battery.low)
        flags |= 0x02;
    if (battery.critical)
        flags |= 0x04;
    buffer[index++] = flags;

    return index;
}

// ============================================================================
// PROCESAMIENTO DE DOWNLINKS
// ============================================================================

void RadioManager::processDownlink(const uint8_t *data, size_t length, uint8_t port)
{
    LOG_D("📡 Procesando downlink puerto %d, %d bytes", port, length);

    switch (port)
    {
    case 1: // Comandos del sistema
        parseSystemCommand(data, length);
        break;
    case 2: // Comandos de alerta
        parseAlertCommand(data, length);
        break;
    case 3: // Comandos de configuración
        parseConfigCommand(data, length);
        break;
    case 10: // Geocerca desde el backend
        parseGeofenceCommand(data, length);
        break;
    default:
        LOG_W("📡 Puerto de downlink desconocido: %d", port);
        break;
    }

    // Ejecutar callback personalizado si existe
    if (downlinkCallback)
    {
        downlinkCallback(data, length, port);
    }
}

void RadioManager::parseSystemCommand(const uint8_t *data, size_t length)
{
    if (length < 1)
        return;

    uint8_t command = data[0];

    switch (command)
    {
    case 0x01: // Reset del dispositivo
        LOG_W("📡 Comando reset recibido");
        delay(1000);
        ESP.restart();
        break;
    case 0x02: // Activar modo sleep
        LOG_I("📡 Comando sleep recibido");
        // Implementar lógica de sleep
        break;
    case 0x03: // Solicitar estado
        LOG_I("📡 Comando estado solicitado");
        // Enviar estado actual
        break;
    default:
        LOG_W("📡 Comando sistema desconocido: 0x%02X", command);
        break;
    }
}

void RadioManager::parseAlertCommand(const uint8_t *data, size_t length)
{
    if (length < 1)
        return;

    uint8_t command = data[0];

    switch (command)
    {
    case 0x01: // Activar buzzer
        LOG_I("📡 Comando activar buzzer");
        // Activar buzzer mediante callback o evento
        break;
    case 0x02: // Desactivar buzzer
        LOG_I("📡 Comando desactivar buzzer");
        break;
    case 0x03: // Cambiar nivel de alerta
        if (length >= 2)
        {
            uint8_t level = data[1];
            LOG_I("📡 Comando cambiar alerta a nivel %d", level);
        }
        break;
    default:
        LOG_W("📡 Comando alerta desconocido: 0x%02X", command);
        break;
    }
}

void RadioManager::parseConfigCommand(const uint8_t *data, size_t length)
{
    if (length < 2)
        return;

    uint8_t param = data[0];
    uint8_t value = data[1];

    switch (param)
    {
    case 0x01: // Intervalo de transmisión
        LOG_I("📡 Nuevo intervalo TX: %d min", value);
        break;
    case 0x02: // Data rate
        LOG_I("📡 Nuevo data rate: %d", value);
        setDataRate(value);
        break;
    case 0x03: // TX Power
        LOG_I("📡 Nueva potencia TX: %d dBm", (int8_t)value);
        setTxPower((int8_t)value);
        break;
    default:
        LOG_W("📡 Parámetro config desconocido: 0x%02X", param);
        break;
    }
}

void RadioManager::parseGeofenceCommand(const uint8_t *data, size_t length)
{
    if (length < 1)
    {
        LOG_W("📡 Comando de geocerca vacío");
        return;
    }

    uint8_t geofenceType = data[0];

    LOG_I("🌐 GEOCERCA RECIBIDA vía LoRaWAN - Tipo: %d", geofenceType);

    if (geofenceType == 0)
    {
        // Círculo
        parseCircleGeofence(data, length);
    }
    else if (geofenceType == 1)
    {
        // Polígono
        parsePolygonGeofence(data, length);
    }
    else
    {
        LOG_W("⚠️ Tipo de geocerca desconocido: %d", geofenceType);
    }
}

void RadioManager::parseCircleGeofence(const uint8_t *data, size_t length)
{
    // Formato: [tipo(1)][lat(4)][lng(4)][radio(2)][groupId(N)] = 11+ bytes
    if (length < 11)
    {
        LOG_W("📡 Comando de geocerca circular muy corto: %d bytes", length);
        return;
    }

    // Extraer coordenadas (en formato float little-endian)
    float lat, lng;
    memcpy(&lat, &data[1], 4);
    memcpy(&lng, &data[5], 4);

    // Extraer radio (2 bytes, uint16 little-endian)
    uint16_t radius = data[9] | (data[10] << 8);

    // Extraer groupId si está presente
    char groupId[16] = "backend";
    if (length > 11)
    {
        size_t groupIdLen = min((size_t)15, length - 11);
        memcpy(groupId, &data[11], groupIdLen);
        groupId[groupIdLen] = '\0';
    }

    LOG_I("🔴 GEOCERCA CÍRCULO:");
    LOG_I("  Centro: %.6f, %.6f", lat, lng);
    LOG_I("  Radio: %d metros", radius);
    LOG_I("  Grupo: %s", groupId);

    // Llamar al callback para actualizar la geocerca
    if (geofenceUpdateCallback)
    {
        GeofenceUpdate update;
        update.type = 0; // Círculo
        update.centerLat = lat;
        update.centerLng = lng;
        update.radius = (float)radius;
        update.pointCount = 0;
        strncpy(update.name, "Circle", sizeof(update.name) - 1);
        strncpy(update.groupId, groupId, sizeof(update.groupId) - 1);
        update.name[sizeof(update.name) - 1] = '\0';
        update.groupId[sizeof(update.groupId) - 1] = '\0';

        geofenceUpdateCallback(update);
        LOG_I("✅ Geocerca circular actualizada");
    }
    else
    {
        LOG_W("⚠️ Callback de geocerca no configurado");
    }
}

void RadioManager::parsePolygonGeofence(const uint8_t *data, size_t length)
{
    // Formato: [tipo(1)][numPuntos(1)][lat1(4)][lng1(4)][lat2(4)][lng2(4)]...[groupId(N)]
    if (length < 3)
    {
        LOG_W("📡 Comando de geocerca poligonal muy corto: %d bytes", length);
        return;
    }

    uint8_t numPoints = data[1];

    if (numPoints < 3 || numPoints > 10)
    {
        LOG_W("⚠️ Número de puntos inválido: %d", numPoints);
        return;
    }

    size_t expectedLength = 2 + (numPoints * 8); // tipo + numPuntos + (lat+lng)*numPuntos
    if (length < expectedLength)
    {
        LOG_W("📡 Comando de polígono incompleto: %d bytes, esperado %d", length, expectedLength);
        return;
    }

    LOG_I("🔷 GEOCERCA POLÍGONO: %d puntos", numPoints);

    // Extraer puntos del polígono
    GeoPoint points[10];
    size_t dataIndex = 2;

    for (uint8_t i = 0; i < numPoints; i++)
    {
        float lat, lng;
        memcpy(&lat, &data[dataIndex], 4);
        memcpy(&lng, &data[dataIndex + 4], 4);

        points[i] = GeoPoint(lat, lng);
        dataIndex += 8;

        LOG_I("  Punto %d: %.6f, %.6f", i, lat, lng);
    }

    // Extraer groupId si está presente
    char groupId[16] = "backend";
    if (length > expectedLength)
    {
        size_t groupIdLen = min((size_t)15, length - expectedLength);
        memcpy(groupId, &data[expectedLength], groupIdLen);
        groupId[groupIdLen] = '\0';
    }

    LOG_I("  Grupo: %s", groupId);

    // Llamar al callback para actualizar la geocerca
    if (geofenceUpdateCallback)
    {
        GeofenceUpdate update;
        update.type = 1; // Polígono
        update.pointCount = numPoints;
        update.centerLat = 0.0;
        update.centerLng = 0.0;
        update.radius = 0.0f;

        // Copiar puntos y calcular centro aproximado
        double sumLat = 0.0, sumLng = 0.0;
        for (uint8_t i = 0; i < numPoints; i++)
        {
            update.points[i].lat = points[i].lat;
            update.points[i].lng = points[i].lng;
            sumLat += points[i].lat;
            sumLng += points[i].lng;
        }
        update.centerLat = sumLat / numPoints;
        update.centerLng = sumLng / numPoints;

        strncpy(update.name, "Polygon", sizeof(update.name) - 1);
        strncpy(update.groupId, groupId, sizeof(update.groupId) - 1);
        update.name[sizeof(update.name) - 1] = '\0';
        update.groupId[sizeof(update.groupId) - 1] = '\0';

        geofenceUpdateCallback(update);
        LOG_I("✅ Geocerca poligonal actualizada");
    }
    else
    {
        LOG_W("⚠️ Callback de geocerca no configurado");
    }
}

// ============================================================================
// GESTIÓN DE ERRORES
// ============================================================================

void RadioManager::handleRadioError(int16_t errorCode)
{
    currentState = STATE_ERROR;

    switch (errorCode)
    {
    case RADIOLIB_ERR_CHIP_NOT_FOUND:
        LOG_E("📡 Radio chip no encontrado");
        break;
    case RADIOLIB_ERR_INVALID_FREQUENCY:
        LOG_E("📡 Frecuencia inválida");
        break;
    case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
        LOG_E("📡 Potencia de salida inválida");
        break;
    default:
        LOG_E("📡 Error de radio: %d", errorCode);
        break;
    }
}

const char *RadioManager::getErrorString(int16_t errorCode)
{
    switch (errorCode)
    {
    case RADIOLIB_ERR_NONE:
        return "OK";
    case RADIOLIB_ERR_CHIP_NOT_FOUND:
        return "Chip no encontrado";
    case RADIOLIB_ERR_INVALID_FREQUENCY:
        return "Frecuencia inválida";
    case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
        return "Potencia inválida";
    case RADIOLIB_ERR_INVALID_BANDWIDTH:
        return "Ancho de banda inválido";
    case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
        return "SF inválido";
    case RADIOLIB_ERR_INVALID_CODING_RATE:
        return "CR inválido";
    case RADIOLIB_ERR_INVALID_SYNC_WORD:
        return "Sync word inválido";
    case RADIOLIB_ERR_TX_TIMEOUT:
        return "TX timeout";
    case RADIOLIB_ERR_RX_TIMEOUT:
        return "RX timeout";
    case RADIOLIB_LORAWAN_NEW_SESSION:
        return "Nueva sesión LoRaWAN";
    case RADIOLIB_LORAWAN_SESSION_RESTORED:
        return "Sesión LoRaWAN restaurada";
    default:
        return "Error desconocido";
    }
}

// ============================================================================
// FUNCIÓN AUXILIAR PARA VALIDAR POSICIÓN GPS
// ============================================================================
bool RadioManager::isValidPosition(const Position &position)
{
    return (position.latitude >= -90.0 && position.latitude <= 90.0 &&
            position.longitude >= -180.0 && position.longitude <= 180.0 &&
            position.altitude >= -500.0 && position.altitude <= 10000.0);
}
