#!/bin/bash
# setup_raspberry.sh - Configuración automática para Raspberry Pi
# Ejecutar con: bash setup_raspberry.sh

echo "================================================"
echo "🚀 CONFIGURACIÓN AUTOMÁTICA RASPBERRY PI"
echo "================================================"

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Variables
BACKEND_DIR="$HOME/projects/lorawan_platform/backend"
DEVICE_EUI=""

# Verificar que estamos en Raspberry Pi
if [ ! -f /etc/os-release ] || ! grep -q "Raspbian" /etc/os-release; then
    echo -e "${YELLOW}⚠️ Advertencia: No parece ser Raspbian${NC}"
fi

echo -e "\n${GREEN}✅ Configuración detectada:${NC}"
echo "   Backend: $BACKEND_DIR"
echo "   ChirpStack: http://localhost:8080"
echo ""

# Solicitar Device EUI
echo -n "Ingrese el Device EUI del ESP32 (16 caracteres hex): "
read DEVICE_EUI

# Validar Device EUI
if [ ${#DEVICE_EUI} -ne 16 ]; then
    echo -e "${RED}❌ Device EUI debe tener 16 caracteres${NC}"
    exit 1
fi

# Crear directorio de scripts si no existe
mkdir -p "$BACKEND_DIR/scripts"

# Copiar script de prueba
echo -e "\n${YELLOW}📝 Instalando script de prueba...${NC}"
cat > "$BACKEND_DIR/scripts/test_geofence.py" << 'EOF'
#!/usr/bin/env python3
import struct
import base64
import requests
import sys

CHIRPSTACK_API_URL = "http://localhost:8080"
CHIRPSTACK_API_TOKEN = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhcGlfa2V5X2lkIjoiNDRhNGE2MzAtMGM4MC00M2EyLTk2NmYtODAwYzQwMzdkOWI4IiwiYXVkIjoiYXMiLCJpc3MiOiJhcyIsIm5iZiI6MTc1NDg3NDMyMSwic3ViIjoiYXBpX2tleSJ9.GDvpbm6rdfj1NivGoxh2ehBeTsJN8-VUuBPnqX6Lios"

def send_geofence(device_eui, lat=-33.4489, lng=-70.6693, radius=100):
    payload = struct.pack('<BffH', 1, float(lat), float(lng), int(radius))
    payload += b'test'
    payload_b64 = base64.b64encode(payload).decode('utf-8')
    
    url = f"{CHIRPSTACK_API_URL}/api/devices/{device_eui}/queue"
    data = {
        "deviceQueueItem": {
            "confirmed": False,
            "data": payload_b64,
            "devEUI": device_eui,
            "fPort": 10
        }
    }
    headers = {
        "Authorization": f"Bearer {CHIRPSTACK_API_TOKEN}",
        "Content-Type": "application/json"
    }
    
    response = requests.post(url, json=data, headers=headers)
    
    if response.status_code in [200, 201, 202]:
        print(f"✅ Geocerca enviada a {device_eui}")
        return True
    else:
        print(f"❌ Error: {response.status_code} - {response.text}")
        return False

if __name__ == "__main__":
    device_eui = sys.argv[1] if len(sys.argv) > 1 else input("Device EUI: ")
    send_geofence(device_eui)
EOF

chmod +x "$BACKEND_DIR/scripts/test_geofence.py"
echo -e "${GREEN}✅ Script de prueba instalado${NC}"

# Instalar dependencias Python
echo -e "\n${YELLOW}📦 Verificando dependencias Python...${NC}"
pip3 install requests python-dotenv asyncpg

# Crear servicio systemd para el backend
echo -e "\n${YELLOW}🔧 Creando servicio systemd...${NC}"
sudo tee /etc/systemd/system/lorawan-backend.service > /dev/null << EOF
[Unit]
Description=LoRaWAN Backend Service
After=network.target postgresql.service

[Service]
Type=simple
User=$USER
WorkingDirectory=$BACKEND_DIR
Environment="PATH=/usr/local/bin:/usr/bin:/bin"
ExecStart=/usr/bin/python3 -m uvicorn app.main:app --host 0.0.0.0 --port 8000
Restart=always

[Install]
WantedBy=multi-user.target
EOF

# Recargar systemd
sudo systemctl daemon-reload
echo -e "${GREEN}✅ Servicio systemd creado${NC}"

# Crear alias útiles
echo -e "\n${YELLOW}🔧 Creando alias útiles...${NC}"
cat >> ~/.bashrc << 'EOF'

# LoRaWAN Aliases
alias lorawan-test='python3 ~/projects/lorawan_platform/backend/scripts/test_geofence.py'
alias lorawan-logs='sudo journalctl -u lorawan-backend -f'
alias lorawan-restart='sudo systemctl restart lorawan-backend'
alias lorawan-status='sudo systemctl status lorawan-backend'
alias chirpstack-logs='sudo journalctl -u chirpstack-application-server -f'
EOF

echo -e "${GREEN}✅ Aliases creados${NC}"

# Test de conexión con ChirpStack
echo -e "\n${YELLOW}🔍 Verificando ChirpStack...${NC}"
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" \
    -H "Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJhcGlfa2V5X2lkIjoiNDRhNGE2MzAtMGM4MC00M2EyLTk2NmYtODAwYzQwMzdkOWI4IiwiYXVkIjoiYXMiLCJpc3MiOiJhcyIsIm5iZiI6MTc1NDg3NDMyMSwic3ViIjoiYXBpX2tleSJ9.GDvpbm6rdfj1NivGoxh2ehBeTsJN8-VUuBPnqX6Lios" \
    "http://localhost:8080/api/internal/profile")

if [ "$RESPONSE" -eq 200 ]; then
    echo -e "${GREEN}✅ ChirpStack funcionando correctamente${NC}"
else
    echo -e "${RED}❌ Error conectando con ChirpStack (HTTP $RESPONSE)${NC}"
fi

# Resumen
echo -e "\n${GREEN}================================================${NC}"
echo -e "${GREEN}✅ INSTALACIÓN COMPLETADA${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""
echo "📝 Comandos disponibles:"
echo "   lorawan-test $DEVICE_EUI    - Enviar geocerca de prueba"
echo "   lorawan-logs                - Ver logs del backend"
echo "   lorawan-restart              - Reiniciar backend"
echo "   lorawan-status               - Estado del backend"
echo "   chirpstack-logs              - Ver logs de ChirpStack"
echo ""
echo "🚀 Para iniciar el backend:"
echo "   sudo systemctl start lorawan-backend"
echo ""
echo "🧪 Para probar el envío de geocerca:"
echo "   python3 $BACKEND_DIR/scripts/test_geofence.py $DEVICE_EUI"
echo ""
echo -e "${YELLOW}⚠️ Recuerda recargar tu shell: source ~/.bashrc${NC}"
