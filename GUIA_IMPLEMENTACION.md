# 📡 GUÍA DE IMPLEMENTACIÓN PASO A PASO

## 🚀 COMANDOS PARA EJECUTAR EN TU RASPBERRY PI

### 1️⃣ Conectar a la Raspberry Pi y actualizar el backend

```bash
# Conectar por SSH
ssh lorawan_app@rak-gateway

# Ir al directorio del backend
cd ~/projects/lorawan_platform/backend

# Hacer backup del archivo actual (por seguridad)
cp app/api/integrations.py app/api/integrations.py.backup

# Crear el nuevo archivo de integrations
nano app/api/integrations.py
# (Pegar el contenido del artifact "Backend Integration Personalizado")
```

### 2️⃣ Instalar el script de prueba

```bash
# Crear directorio de scripts
mkdir -p ~/projects/lorawan_platform/backend/scripts

# Crear script de prueba
nano ~/projects/lorawan_platform/backend/scripts/test_geofence.py
# (Pegar el contenido del artifact "Script de Prueba para tu Raspberry Pi")

# Dar permisos de ejecución
chmod +x ~/projects/lorawan_platform/backend/scripts/test_geofence.py

# Instalar dependencias si faltan
pip3 install requests
```

### 3️⃣ Configurar el codec en ChirpStack

```bash
# Abrir navegador en tu PC
http://[IP-RASPBERRY]:8080

# Login ChirpStack
Usuario: admin
Password: admin (o la que hayas configurado)

# Navegar a:
1. Applications → [Tu Aplicación]
2. Device profiles → [Tu Perfil]
3. Codec → Custom JavaScript codec functions
4. Pegar el código del artifact "Codec JavaScript para ChirpStack"
5. Save
```

### 4️⃣ Probar el envío de geocerca

```bash
# Obtener el Device EUI de tu ESP32
# (Debería aparecer en el monitor serial del ESP32 al iniciar)

# Probar envío (reemplaza DEVICE_EUI con el valor real)
python3 ~/projects/lorawan_platform/backend/scripts/test_geofence.py 70b3d57ed8003421

# O con parámetros personalizados
python3 test_geofence.py 70b3d57ed8003421 --lat -33.4489 --lng -70.6693 --radius 150
```

### 5️⃣ Verificar en el ESP32

En el monitor serial del ESP32 deberías ver:

```
🌐 GEOCERCA RECIBIDA vía LoRaWAN - Tipo: 1
🔴 GEOCERCA CÍRCULO:
  Centro: -33.448900, -70.669300
  Radio: 100 metros
  Grupo: test
✅ Geocerca circular actualizada
💾 Geocerca guardada en NVS
```

### 6️⃣ Configurar el webhook (opcional)

```bash
# En ChirpStack UI
Applications → [Tu App] → Integrations → HTTP

# Configurar:
Uplink data URL: http://localhost:8000/api/integrations/webhook/uplink
Headers:
  Content-Type: application/json
```

### 7️⃣ Reiniciar el backend con los cambios

```bash
# Si está corriendo con uvicorn
pkill -f uvicorn

# Iniciar nuevamente
cd ~/projects/lorawan_platform/backend
python3 -m uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload &

# Ver logs
tail -f uvicorn.log
```

## 🔍 VERIFICACIÓN Y DEBUGGING

### Verificar conexión con ChirpStack:

```bash
curl -H "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhcGlfa2V5X2lkIjoiNDRhNGE2MzAtMGM4MC00M2EyLTk2NmYtODAwYzQwMzdkOWI4IiwiYXVkIjoiYXMiLCJpc3MiOiJhcyIsIm5iZiI6MTc1NDg3NDMyMSwic3ViIjoiYXBpX2tleSJ9.GDvpbm6rdfj1NivGoxh2ehBeTsJN8-VUuBPnqX6Lios" \
  http://localhost:8080/api/internal/profile
```

### Ver cola de downlinks:

```bash
curl -H "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhcGlfa2V5X2lkIjoiNDRhNGE2MzAtMGM4MC00M2EyLTk2NmYtODAwYzQwMzdkOWI4IiwiYXVkIjoiYXMiLCJpc3MiOiJhcyIsIm5iZiI6MTc1NDg3NDMyMSwic3ViIjoiYXBpX2tleSJ9.GDvpbm6rdfj1NivGoxh2ehBeTsJN8-VUuBPnqX6Lios" \
  http://localhost:8080/api/devices/[DEVICE_EUI]/queue
```

### Ver eventos del dispositivo:

```bash
curl -H "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhcGlfa2V5X2lkIjoiNDRhNGE2MzAtMGM4MC00M2EyLTk2NmYtODAwYzQwMzdkOWI4IiwiYXVkIjoiYXMiLCJpc3MiOiJhcyIsIm5iZiI6MTc1NDg3NDMyMSwic3ViIjoiYXBpX2tleSJ9.GDvpbm6rdfj1NivGoxh2ehBeTsJN8-VUuBPnqX6Lios" \
  http://localhost:8080/api/devices/[DEVICE_EUI]/events?limit=10 | python3 -m json.tool
```

## ⚠️ SOLUCIÓN DE PROBLEMAS

### Error: Device not found
- Verificar que el Device EUI sea correcto (16 caracteres hex)
- Verificar que el dispositivo esté registrado en ChirpStack
- El Device EUI debe estar en minúsculas sin separadores

### Error: 401 Unauthorized
- El token puede haber expirado
- Crear un nuevo token en ChirpStack UI → API Keys

### Downlink no llega al ESP32
1. Verificar que el ESP32 esté enviando uplinks
2. El downlink se entrega DESPUÉS de un uplink
3. Verificar en ChirpStack UI → Device → LoRaWAN Frames

### Error de conexión con ChirpStack
```bash
# Verificar que ChirpStack esté corriendo
sudo systemctl status chirpstack-application-server

# Ver logs
sudo journalctl -u chirpstack-application-server -f

# Reiniciar si es necesario
sudo systemctl restart chirpstack-application-server
```

## 📊 MONITOREO EN TIEMPO REAL

### Terminal 1: Monitor del ESP32
```bash
# En tu PC con PlatformIO
pio device monitor
```

### Terminal 2: Logs del backend
```bash
# En Raspberry Pi
tail -f ~/projects/lorawan_platform/backend/uvicorn.log
```

### Terminal 3: Logs de ChirpStack
```bash
# En Raspberry Pi
sudo journalctl -u chirpstack-application-server -f
```

### Terminal 4: Logs del Gateway
```bash
# En Raspberry Pi
sudo journalctl -u chirpstack-gateway-bridge -f
```

## ✅ CHECKLIST DE VERIFICACIÓN

- [ ] Backend actualizado con el nuevo código
- [ ] Script de prueba instalado
- [ ] Codec JavaScript configurado en ChirpStack
- [ ] Test de downlink exitoso
- [ ] Geocerca visible en monitor serial del ESP32
- [ ] Geocerca persiste después de reiniciar ESP32
- [ ] Webhook configurado (opcional)
- [ ] Backend reiniciado con nuevos cambios

## 🎯 PRUEBA COMPLETA END-TO-END

1. **Resetear ESP32** para empezar limpio
2. **Enviar geocerca de prueba:**
   ```bash
   python3 test_geofence.py [DEVICE_EUI] --lat -33.45 --lng -70.67 --radius 200
   ```
3. **Esperar próximo uplink** del ESP32 (máximo 60 segundos)
4. **Verificar en monitor serial** que se recibió la geocerca
5. **Mover el ESP32** fuera del radio para activar alerta
6. **Verificar alerta** en el monitor serial
7. **Reiniciar ESP32** y verificar que la geocerca se mantiene

## 📝 NOTAS IMPORTANTES

- El ESP32 debe estar enviando uplinks para recibir downlinks
- Los downlinks se entregan en la ventana RX después de un uplink
- El puerto 10 está reservado para geocercas
- Máximo payload: 51 bytes en DR0, 115 bytes en DR3
- Las geocercas se guardan automáticamente en NVS del ESP32

---
**Última actualización:** Enero 2025
**Tu Token ChirpStack:** Válido y funcionando
**Tu Device EUI:** Por configurar en las pruebas
