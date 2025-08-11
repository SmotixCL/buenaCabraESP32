# ARCHIVO COMPLETO: app/api/geofences.py
# Copia este archivo completo en tu Raspberry Pi

from fastapi import APIRouter, Depends, HTTPException, BackgroundTasks
from sqlalchemy.ext.asyncio import AsyncSession
from typing import List
from app.dependencies import get_db
from app.schemas import geofence as schemas  # Importación correcta
from app.services import geofence_service, group_service
import struct
import requests
import base64
import logging

# Configurar logging
logger = logging.getLogger(__name__)

router = APIRouter()

# ============================================================================
# FUNCIÓN PARA ENVIAR DOWNLINK DE GEOCERCA
# ============================================================================

async def send_geofence_downlink(device_eui: str, lat: float, lng: float, radius: int):
    """
    Envía una geocerca como downlink al dispositivo vía ChirpStack v3
    """
    try:
        # Formato del payload: [tipo(1)][lat(4)][lng(4)][radio(2)] = 11 bytes
        payload = struct.pack('<BffH', 
            1,  # Tipo: 1 = círculo
            lat,  # Latitud como float (4 bytes)
            lng,  # Longitud como float (4 bytes)  
            radius  # Radio como uint16 (2 bytes)
        )
        
        # Codificar en base64
        payload_b64 = base64.b64encode(payload).decode('utf-8')
        
        # URL de la API de ChirpStack v3
        # IMPORTANTE: Cambiar según tu configuración
        chirpstack_url = "http://localhost:8080/api/devices/{}/queue"
        
        # Datos del downlink para ChirpStack v3
        downlink_data = {
            "deviceQueueItem": {
                "confirmed": False,
                "data": payload_b64,
                "devEUI": device_eui,
                "fPort": 10  # Puerto 10 para geocercas
            }
        }
        
        # Headers con API token
        # IMPORTANTE: Reemplaza con tu token real de ChirpStack
        # Para obtenerlo: ChirpStack UI > API Keys > Create
        headers = {
            "Authorization": "Bearer YOUR_CHIRPSTACK_API_TOKEN",  # <-- CAMBIA ESTO
            "Content-Type": "application/json"
        }
        
        response = requests.post(
            chirpstack_url.format(device_eui),
            json=downlink_data,
            headers=headers,
            timeout=5
        )
        
        if response.status_code == 200:
            logger.info(f"✅ Geocerca enviada a {device_eui}: {lat},{lng} R={radius}m")
            print(f"✅ Geocerca enviada a {device_eui}: {lat},{lng} R={radius}m")
            return True
        else:
            logger.error(f"❌ Error enviando geocerca: {response.status_code} - {response.text}")
            print(f"❌ Error enviando geocerca: {response.status_code} - {response.text}")
            return False
            
    except requests.exceptions.Timeout:
        logger.error(f"❌ Timeout enviando geocerca a {device_eui}")
        return False
    except Exception as e:
        logger.error(f"❌ Excepción enviando geocerca: {e}")
        print(f"❌ Excepción enviando geocerca: {e}")
        return False

# ============================================================================
# ENDPOINTS ORIGINALES (NO MODIFICAR SI YA FUNCIONAN)
# ============================================================================

@router.get("/", response_model=List[schemas.Geofence])
async def read_geofences(skip: int = 0, limit: int = 100, db: AsyncSession = Depends(get_db)):
    geofences = await geofence_service.get_geofences(db, skip=skip, limit=limit)
    return geofences

@router.get("/{geofence_id}", response_model=schemas.Geofence)
async def read_geofence(geofence_id: int, db: AsyncSession = Depends(get_db)):
    db_geofence = await geofence_service.get_geofence(db, geofence_id)
    if db_geofence is None:
        raise HTTPException(status_code=404, detail="Geofence not found")
    return db_geofence

# ============================================================================
# CREAR GEOCERCA CON DOWNLINK AUTOMÁTICO
# ============================================================================

@router.post("/", response_model=schemas.Geofence)
async def create_geofence_endpoint(
    geofence: schemas.GeofenceCreate,
    background_tasks: BackgroundTasks,
    db: AsyncSession = Depends(get_db)
):
    """
    Crea una nueva geocerca y envía downlink a dispositivos
    """
    # Crear geocerca en base de datos
    db_geofence = await geofence_service.create_geofence(db, geofence)
    
    try:
        # Obtener grupo asociado
        group = await group_service.get_group(db, geofence.group_id)
        
        if group and group.devices:
            # Determinar coordenadas según el tipo
            center_lat = 0.0
            center_lng = 0.0
            radius = 100  # Radio por defecto
            
            if geofence.geofence_type == "circle":
                # Para círculo, coordinates debe tener lat, lng, radius
                if hasattr(geofence.coordinates, 'lat'):
                    center_lat = geofence.coordinates.lat
                    center_lng = geofence.coordinates.lng
                    radius = getattr(geofence.coordinates, 'radius', 100)
                elif isinstance(geofence.coordinates, dict):
                    center_lat = geofence.coordinates.get('lat', 0)
                    center_lng = geofence.coordinates.get('lng', 0)
                    radius = geofence.coordinates.get('radius', 100)
            else:
                # Para polígono, calcular centro
                if isinstance(geofence.coordinates, list) and len(geofence.coordinates) > 0:
                    # Calcular centroide del polígono
                    lats = [c.lat if hasattr(c, 'lat') else c.get('lat', 0) for c in geofence.coordinates]
                    lngs = [c.lng if hasattr(c, 'lng') else c.get('lng', 0) for c in geofence.coordinates]
                    center_lat = sum(lats) / len(lats) if lats else 0
                    center_lng = sum(lngs) / len(lngs) if lngs else 0
                    
                    # Calcular radio aproximado (distancia máxima desde el centro)
                    max_dist = 0
                    for lat, lng in zip(lats, lngs):
                        dist = ((lat - center_lat)**2 + (lng - center_lng)**2) ** 0.5
                        max_dist = max(max_dist, dist)
                    radius = int(max_dist * 111000)  # Aproximación: 1 grado = 111km
            
            # Enviar downlink a cada dispositivo del grupo
            for device in group.devices:
                logger.info(f"Enviando geocerca a dispositivo {device.dev_eui}")
                background_tasks.add_task(
                    send_geofence_downlink,
                    device.dev_eui,
                    float(center_lat),
                    float(center_lng),
                    int(radius)
                )
        else:
            logger.warning(f"Grupo {geofence.group_id} sin dispositivos")
            
    except Exception as e:
        logger.error(f"Error procesando downlinks: {e}")
        # No fallar la creación de la geocerca por error en downlinks
        pass
    
    return db_geofence

# ============================================================================
# ACTUALIZAR GEOCERCA CON DOWNLINK AUTOMÁTICO
# ============================================================================

@router.put("/{geofence_id}", response_model=schemas.Geofence)
async def update_geofence_endpoint(
    geofence_id: int,
    geofence: schemas.GeofenceCreate,
    background_tasks: BackgroundTasks,
    db: AsyncSession = Depends(get_db)
):
    """
    Actualiza una geocerca existente y envía downlink a dispositivos
    """
    # Actualizar en base de datos
    db_geofence = await geofence_service.update_geofence(db, geofence_id, geofence)
    if not db_geofence:
        raise HTTPException(status_code=404, detail="Geofence not found")
    
    try:
        # Obtener grupo asociado
        group = await group_service.get_group(db, geofence.group_id)
        
        if group and group.devices:
            # Misma lógica de extracción de coordenadas que en create
            center_lat = 0.0
            center_lng = 0.0
            radius = 100
            
            if geofence.geofence_type == "circle":
                if hasattr(geofence.coordinates, 'lat'):
                    center_lat = geofence.coordinates.lat
                    center_lng = geofence.coordinates.lng
                    radius = getattr(geofence.coordinates, 'radius', 100)
                elif isinstance(geofence.coordinates, dict):
                    center_lat = geofence.coordinates.get('lat', 0)
                    center_lng = geofence.coordinates.get('lng', 0)
                    radius = geofence.coordinates.get('radius', 100)
            else:
                if isinstance(geofence.coordinates, list) and len(geofence.coordinates) > 0:
                    lats = [c.lat if hasattr(c, 'lat') else c.get('lat', 0) for c in geofence.coordinates]
                    lngs = [c.lng if hasattr(c, 'lng') else c.get('lng', 0) for c in geofence.coordinates]
                    center_lat = sum(lats) / len(lats) if lats else 0
                    center_lng = sum(lngs) / len(lngs) if lngs else 0
                    
                    max_dist = 0
                    for lat, lng in zip(lats, lngs):
                        dist = ((lat - center_lat)**2 + (lng - center_lng)**2) ** 0.5
                        max_dist = max(max_dist, dist)
                    radius = int(max_dist * 111000)
            
            # Enviar downlink actualizado
            for device in group.devices:
                logger.info(f"Actualizando geocerca en dispositivo {device.dev_eui}")
                background_tasks.add_task(
                    send_geofence_downlink,
                    device.dev_eui,
                    float(center_lat),
                    float(center_lng),
                    int(radius)
                )
                
    except Exception as e:
        logger.error(f"Error procesando downlinks en update: {e}")
        pass
    
    return db_geofence

# ============================================================================
# ELIMINAR GEOCERCA
# ============================================================================

@router.delete("/{geofence_id}", status_code=204)
async def delete_geofence_endpoint(geofence_id: int, db: AsyncSession = Depends(get_db)):
    """
    Elimina una geocerca
    Nota: No se envía downlink de eliminación por ahora
    """
    success = await geofence_service.delete_geofence(db, geofence_id)
    if not success:
        raise HTTPException(status_code=404, detail="Geofence not found")
    return {"ok": True}

# ============================================================================
# ENDPOINTS ADICIONALES
# ============================================================================

@router.get("/group/{group_id}", response_model=List[schemas.Geofence])
async def read_geofences_by_group(group_id: int, db: AsyncSession = Depends(get_db)):
    """
    Obtiene todas las geocercas de un grupo
    """
    geofences = await geofence_service.get_geofences_by_group(db, group_id)
    return geofences
