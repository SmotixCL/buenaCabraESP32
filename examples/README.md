# 📂 Ejemplos de Configuración Principal

Esta carpeta contiene versiones alternativas del archivo `main.cpp` para diferentes casos de uso:

## 📄 Archivos Disponibles:

### 1. **main_minimal.cpp.example**
- Versión minimalista del código principal
- Útil para pruebas básicas de hardware
- Incluye solo funcionalidades esenciales:
  - GPS básico
  - LoRaWAN simple
  - Display OLED
  - Sin geocercas complejas

### 2. **main_test.cpp.example**
- Versión de prueba con funciones de diagnóstico
- Incluye:
  - Pruebas de hardware
  - Mensajes de debug detallados
  - Funciones de calibración
  - Modo de prueba sin LoRaWAN

## 🚀 Cómo Usar:

Para usar una versión alternativa:

1. **Hacer backup del main.cpp actual:**
   ```bash
   cp src/main.cpp src/main.cpp.backup
   ```

2. **Copiar el ejemplo deseado:**
   ```bash
   cp examples/main_minimal.cpp.example src/main.cpp
   ```
   O para el test:
   ```bash
   cp examples/main_test.cpp.example src/main.cpp
   ```

3. **Compilar y cargar:**
   ```bash
   pio run --target upload
   ```

4. **Para volver a la versión original:**
   ```bash
   cp src/main.cpp.backup src/main.cpp
   ```

## ⚠️ Notas Importantes:

- **NO** renombres estos archivos a `.cpp` en esta carpeta o se compilarán automáticamente
- La extensión `.example` evita que PlatformIO los incluya en la compilación
- Solo debe haber UN archivo main.cpp activo en la carpeta `src/`

## 📊 Comparación de Versiones:

| Característica | main.cpp (Principal) | main_minimal | main_test |
|---------------|---------------------|--------------|-----------|
| LoRaWAN Completo | ✅ | ✅ | ❌ |
| Geocercas | ✅ | ❌ | ✅ |
| Alertas Multinivel | ✅ | ❌ | ✅ |
| Display Completo | ✅ | ✅ | ✅ |
| Modo Debug | ✅ | ❌ | ✅ |
| Consumo Energía | Normal | Bajo | Alto |
| Tamaño Flash | ~75% | ~40% | ~60% |

## 🔧 Configuración Recomendada:

- **Producción**: Usar `main.cpp` principal
- **Pruebas de Campo**: Usar `main_minimal.cpp.example`
- **Diagnóstico**: Usar `main_test.cpp.example`

---
*Documentación generada para el proyecto Collar LoRaWAN v3.0*
