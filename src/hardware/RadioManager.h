#pragma once
#include <Arduino.h>
#include "../config/pins.h"
#include "../config/constants.h"
#include "../config/lorawan_config.h"
#include "../core/Types.h"
#include <RadioLib.h>
#ifdef USE_PREFERENCES
#include <Preferences.h> // Para persistencia de DevNonce y Frame Counters
#endif

// La estructura GeofenceUpdate ya está definida en lorawan_config.h
typedef void (*GeofenceUpdateCallback)(const GeofenceUpdate &update);

// Constantes adicionales
#ifndef SPI_FREQUENCY
#define SPI_FREQUENCY 8000000
#endif

// Flags para payload GPS
#define DEVICE_GPS_FIX_FLAG 0x01
#define DEVICE_BATTERY_LOW_FLAG 0x02
#define GEOFENCE_INSIDE_FLAG 0x04
#define GEOFENCE_TYPE_MASK 0x0F
#define GEOFENCE_ACTIVE_FLAG 0x10

// Estructura para payload GPS V2 simplificado
struct __attribute__((packed)) GPSPayloadV2
{
    uint8_t messageType;    // 0x01 = GPS position
    int32_t latitude;       // Latitud * 10000000
    int32_t longitude;      // Longitud * 10000000
    uint16_t altitude;      // Altitud en metros
    uint8_t hdop;           // HDOP * 10
    uint8_t battery;        // Batería %
    uint8_t alert;          // Nivel de alerta
    uint8_t batteryPercent; // Alias para compatibilidad
    uint8_t status;         // Flags de estado
    uint8_t satellites;     // Número de satélites
    uint8_t groupIdHash;    // Hash del groupId
    uint8_t geofenceFlags;  // Flags de geocerca
    uint8_t frameCounter;   // Contador de frames
};

/*
 * ============================================================================
 * RADIO MANAGER - GESTIÓN LORA/LORAWAN
 * ============================================================================
 */

class RadioManager
{
public:
    RadioManager(uint8_t nss = LORA_NSS, uint8_t dio1 = LORA_DIO1,
                 uint8_t rst = LORA_RST, uint8_t busy = LORA_BUSY);

    // Inicialización
    Result init();
    bool isInitialized() const;

    // Configuración básica LoRa
    Result setupLoRa(float frequency = 915.0, float bandwidth = 125.0,
                     uint8_t spreadingFactor = 9, uint8_t codingRate = 7,
                     int8_t power = 20);

    // LoRaWAN (ABP/OTAA)
    Result setupLoRaWAN();
    Result joinOTAA(const uint8_t *devEUI, const uint8_t *appEUI, const uint8_t *appKey);
    Result joinABP(const uint8_t *devAddr, const uint8_t *nwkSKey, const uint8_t *appSKey);
    bool isJoined() const;
    Result forceRejoin();

    // Transmisión de datos
    Result sendPacket(const uint8_t *data, size_t length, uint8_t port = 1);
    Result sendString(const String &message, uint8_t port = 1);
    Result sendPosition(const Position &position, AlertLevel alertLevel = AlertLevel::SAFE);
    Result sendBatteryStatus(const BatteryStatus &battery);

    // Recepción de datos (downlinks)
    Result receivePacket(uint8_t *buffer, size_t *length, uint8_t *port = nullptr);
    bool hasDownlink() const;
    void processDownlinks(); // Llamar desde loop principal

    // Estadísticas y estado
    uint16_t getPacketsSent() const;
    uint16_t getPacketsReceived() const;
    uint16_t getPacketsLost() const;
    float getRSSI() const;
    float getSNR() const;

    // Configuración avanzada
    void setDataRate(uint8_t dataRate);
    void setTxPower(int8_t power);
    void setAdaptiveDataRate(bool enabled);
    void setConfirmedUplinks(bool enabled);

    // Callbacks para eventos
    typedef void (*DownlinkCallback)(const uint8_t *data, size_t length, uint8_t port);
    typedef void (*JoinCallback)(bool success);
    typedef void (*TxCallback)(bool success);

    void setDownlinkCallback(DownlinkCallback callback);
    void setJoinCallback(JoinCallback callback);
    void setTxCallback(TxCallback callback);

    // 🔥 NUEVO: Callback para actualizaciones de geocerca
    void setGeofenceUpdateCallback(GeofenceUpdateCallback callback);

    // Estados del radio
    enum RadioState
    {
        STATE_IDLE,
        STATE_TX,
        STATE_RX,
        STATE_JOINING,
        STATE_JOINED,
        STATE_ERROR
    };

    RadioState getState() const;
    const char *getStateString() const;

    // Gestión de energía
    void sleep();
    void wakeup();
    bool isSleeping() const;

private:
    // Hardware
    SX1262 radio;
    LoRaWANNode lorawan;

    // Configuración
    uint8_t nssPin, dio1Pin, rstPin, busyPin;
    bool initialized;
    bool joined;
    bool sleeping;
    RadioState currentState;

    // Estadísticas con contadores corregidos
    uint16_t packetsSent;
    uint16_t packetsReceived;
    uint16_t packetsLost;
    float lastRSSI;
    float lastSNR;

    // NUEVO: Contadores de frame para verificación
    uint16_t uplinkFrameCounter;
    uint16_t downlinkFrameCounter;
    uint32_t lastUplinkTime;
    uint32_t lastDownlinkTime;

    // Configuración LoRaWAN
    uint8_t currentDataRate;
    int8_t currentTxPower;
    bool adrEnabled;
    bool confirmedUplinks;

    // Buffers de comunicación
    static const size_t MAX_PAYLOAD_SIZE = 51; // LoRaWAN max para DR0
    uint8_t txBuffer[MAX_PAYLOAD_SIZE];
    uint8_t rxBuffer[MAX_PAYLOAD_SIZE];

    // Callbacks
    DownlinkCallback downlinkCallback;
    JoinCallback joinCallback;
    TxCallback txCallback;

    // 🔥 NUEVO: Callback estático para actualizaciones de geocerca
    static GeofenceUpdateCallback geofenceUpdateCallback;

    // Variables para manejo de downlinks
    bool pendingDownlink;
    size_t downlinkLength;
    uint8_t downlinkPort;

    // Métodos privados
    void setupSPI();
    void resetRadio();
    Result configureRadio();

    // Manejo de interrupciones
    static void onDio1Action();
    static RadioManager *instance; // Para callback estático
    void handleDio1Interrupt();

    // Utilidades LoRaWAN
    size_t createPositionPayload(uint8_t *buffer, const Position &position, AlertLevel alertLevel);
    size_t createBatteryPayload(uint8_t *buffer, const BatteryStatus &battery);
    bool isValidPosition(const Position &position);
    static uint8_t calculateGroupHash(const char *groupId);

    // NUEVO: Payload mejorado con estado del dispositivo
    size_t createDeviceStatusPayload(uint8_t *buffer, const Position &position,
                                     const BatteryStatus &battery, AlertLevel alertLevel,
                                     const Geofence &geofence, bool gpsValid,
                                     bool insideGeofence, uint8_t frameCount);

    // Procesamiento de downlinks
    void processDownlink(const uint8_t *data, size_t length, uint8_t port);
    void parseSystemCommand(const uint8_t *data, size_t length);
    void parseAlertCommand(const uint8_t *data, size_t length);
    void parseConfigCommand(const uint8_t *data, size_t length);
    void parseGeofenceCommand(const uint8_t *data, size_t length);

    // NUEVO: Decodificación específica para polígonos
    void parseCircleGeofence(const uint8_t *data, size_t length);
    void parsePolygonGeofence(const uint8_t *data, size_t length);

    // Gestión de errores
    void handleRadioError(int16_t errorCode);
    const char *getErrorString(int16_t errorCode);

    // Nuevos métodos para persistencia de sesión
    bool saveSessionState();
    bool loadSessionState();
    bool isSessionValid();
    void clearSessionState();

    // Variables para tracking de sesión
    uint32_t sessionStartTime;
    uint32_t lastSuccessfulUplink;
    bool sessionRestored;
};

// ============================================================================
// CONFIGURACIÓN LORAWAN EXTERNA
// ============================================================================
struct LoRaWANConfig
{
    // OTAA
    uint8_t devEUI[8];
    uint8_t appEUI[8];
    uint8_t appKey[16];

    // ABP
    uint8_t devAddr[4];
    uint8_t nwkSKey[16];
    uint8_t appSKey[16];

    // Configuración
    bool useOTAA;
    uint8_t region;  // 0=EU868, 1=US915, 2=AU915
    uint8_t subBand; // Para US915/AU915
    uint8_t defaultDataRate;
    int8_t defaultTxPower;
    bool adrEnabled;
    bool confirmedUplinks;

    // NUEVO: Frame counters para tracking
    uint16_t upFrameCounter;
    uint16_t downFrameCounter;

    LoRaWANConfig() : useOTAA(true), region(2), subBand(1), // AU915 sub-banda 1 para Chile
                      defaultDataRate(0), defaultTxPower(20),
                      adrEnabled(true), confirmedUplinks(false),
                      upFrameCounter(0), downFrameCounter(0)
    {
        memset(devEUI, 0, 8);
        memset(appEUI, 0, 8);
        memset(appKey, 0, 16);
        memset(devAddr, 0, 4);
        memset(nwkSKey, 0, 16);
        memset(appSKey, 0, 16);
    }
};
