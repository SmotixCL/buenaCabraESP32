# Instrucciones para configurar Downlinks de Geocerca en Raspberry Pi

## Cambios en el Backend (app/api/integrations.py)

Necesitas agregar código para enviar downlinks con geocercas cuando se creen o actualicen desde la web.

### 1. Agregar función para enviar downlink de geocerca

En el archivo `app/api/integrations.py` del backend en la Raspberry, agrega esta función:

```python
import struct
import requests
import base64

async def send_geofence_downlink(device_eui: str, lat: float, lng: float, radius: int):
    """
    Envía una geocerca como downlink al dispositivo vía ChirpStack
    """
    # Formato del payload: [tipo(1)][lat(4)][lng(4)][radio(2)] = 11 bytes
    payload = struct.pack('<BffH', 
        1,  # Tipo: 1 = círculo
        lat,  # Latitud como float (4 bytes)
        lng,  # Longitud como float (4 bytes)  
        radius  # Radio como uint16 (2 bytes)
    )
    
    # Codificar en base64
    payload_b64 = base64.b64encode(payload).decode('utf-8')
    
    # URL de la API de ChirpStack
    chirpstack_url = "http://localhost:8080/api/devices/{}/queue"
    
    # Datos del downlink
    downlink_data = {
        "deviceQueueItem": {
            "confirmed": False,
            "data": payload_b64,
            "devEUI": device_eui,
            "fPort": 10  # Puerto 10 para geocercas
        }
    }
    
    # Headers con API token
    headers = {
        "Authorization": "Bearer YOUR_CHIRPSTACK_API_TOKEN",
        "Content-Type": "application/json"
    }
    
    try:
        response = requests.post(
            chirpstack_url.format(device_eui),
            json=downlink_data,
            headers=headers
        )
        
        if response.status_code == 200:
            print(f"✅ Geocerca enviada a {device_eui}: {lat},{lng} R={radius}m")
            return True
        else:
            print(f"❌ Error enviando geocerca: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Excepción enviando geocerca: {e}")
        return False
```

### 2. Modificar el endpoint de creación de geocerca

En `app/api/geofences.py`, modifica el endpoint para enviar automáticamente el downlink:

```python
@router.post("/", response_model=schemas.Geofence)
async def create_geofence(
    geofence: schemas.GeofenceCreate,
    background_tasks: BackgroundTasks,
    db: AsyncSession = Depends(get_db)
):
    db_geofence = await geofence_service.create_geofence(db, geofence)
    
    # Obtener dispositivos del grupo
    group = await group_service.get_group(db, geofence.group_id)
    
    # Enviar downlink a cada dispositivo del grupo
    for device in group.devices:
        background_tasks.add_task(
            send_geofence_downlink,
            device.dev_eui,
            db_geofence.coordinates['lat'] if isinstance(db_geofence.coordinates, dict) 
                else db_geofence.coordinates[0]['lat'],
            db_geofence.coordinates['lng'] if isinstance(db_geofence.coordinates, dict)
                else db_geofence.coordinates[0]['lng'],
            int(db_geofence.radius)
        )
    
    return db_geofence
```

### 3. Hacer lo mismo para el endpoint de actualización

```python
@router.put("/{geofence_id}", response_model=schemas.Geofence)
async def update_geofence(
    geofence_id: int,
    geofence: schemas.GeofenceCreate,
    background_tasks: BackgroundTasks,
    db: AsyncSession = Depends(get_db)
):
    db_geofence = await geofence_service.update_geofence(db, geofence_id, geofence)
    if not db_geofence:
        raise HTTPException(status_code=404, detail="Geofence not found")
    
    # Obtener dispositivos del grupo
    group = await group_service.get_group(db, geofence.group_id)
    
    # Enviar downlink actualizado
    for device in group.devices:
        background_tasks.add_task(
            send_geofence_downlink,
            device.dev_eui,
            db_geofence.coordinates['lat'] if isinstance(db_geofence.coordinates, dict)
                else db_geofence.coordinates[0]['lat'],
            db_geofence.coordinates['lng'] if isinstance(db_geofence.coordinates, dict)
                else db_geofence.coordinates[0]['lng'],
            int(db_geofence.radius)
        )
    
    return db_geofence
```

## Configuración de ChirpStack para Downlinks

### 1. Obtener API Token de ChirpStack

```bash
# Acceder a ChirpStack UI
http://raspberry-ip:8080

# Ir a API Keys y crear un nuevo token
# Copiar el token y reemplazar YOUR_CHIRPSTACK_API_TOKEN en el código
```

### 2. Configurar el Codec en ChirpStack

En ChirpStack, ve a tu Application > Device Profile y agrega este codec JavaScript:

```javascript
// Decode uplink
function decodeUplink(input) {
    var data = {};
    
    // Tu decodificación actual...
    
    return {
        data: data
    };
}

// Encode downlink (nuevo)
function encodeDownlink(input) {
    var bytes = [];
    
    if (input.data.type === "geofence") {
        // Tipo de geocerca (1 = círculo)
        bytes.push(1);
        
        // Latitud (4 bytes, little endian float)
        var latBytes = floatToBytes(input.data.lat);
        bytes = bytes.concat(latBytes);
        
        // Longitud (4 bytes, little endian float)
        var lngBytes = floatToBytes(input.data.lng);
        bytes = bytes.concat(lngBytes);
        
        // Radio (2 bytes, little endian uint16)
        bytes.push(input.data.radius & 0xFF);
        bytes.push((input.data.radius >> 8) & 0xFF);
    }
    
    return {
        bytes: bytes,
        fPort: 10
    };
}

function floatToBytes(float) {
    var buffer = new ArrayBuffer(4);
    var view = new DataView(buffer);
    view.setFloat32(0, float, true); // true = little endian
    return [
        view.getUint8(0),
        view.getUint8(1),
        view.getUint8(2),
        view.getUint8(3)
    ];
}
```

## Test Manual de Downlink

Para probar manualmente el envío de geocerca:

```bash
# En la Raspberry Pi, usar mosquitto_pub
mosquitto_pub -h localhost -t "application/1/device/YOUR_DEV_EUI/command/down" -m '{
  "confirmed": false,
  "fPort": 10,
  "data": "AQBApsMAAKDDAABIAA=="
}'

# El data es base64 de: [1][lat][lng][radius]
# Ejemplo: tipo=1, lat=-33.4489, lng=-70.6693, radius=100m
```

## Verificación

1. Crear una geocerca desde la web
2. Verificar en los logs del ESP32 que se recibe el mensaje:
   ```
   🌐 GEOCERCA RECIBIDA vía LoRaWAN:
     Centro: -33.448900, -70.669300
     Radio: 100 metros
   ```
3. La geocerca debe persistir después de reiniciar el ESP32

## Notas importantes

- El puerto 10 está reservado para geocercas
- Las geocercas se guardan automáticamente en la memoria del ESP32
- Si el dispositivo está offline, ChirpStack encolará el downlink hasta que se conecte
- Máximo tamaño de payload: 51 bytes en DR0, 115 bytes en DR3

## Troubleshooting

Si no llegan los downlinks:

1. Verificar que el dispositivo esté haciendo uplinks (para abrir ventana RX)
2. Verificar en ChirpStack UI > Device > LoRaWAN Frames que se envíe el downlink
3. Verificar que el puerto sea 10
4. Revisar logs del gateway: `sudo journalctl -u chirpstack-gateway-bridge -f`
