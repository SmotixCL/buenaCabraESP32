# 🐛 Debugging Files

Este directorio contiene archivos utilizados durante el proceso de debugging y desarrollo.

## Archivos

### `main_backup.cpp`
- **Código original completo** del sistema collar V3.0
- **Problema**: Causaba reinicios constantes y bloqueo de Serial
- **Estado**: Funcional pero inestable
- **Uso**: Referencia histórica y análisis de problemas

## Historia del Debugging

El sistema original era completamente funcional pero tenía problemas de estabilidad:

1. **Reinicios constantes** durante la operación
2. **Serial output bloqueado** - logs no aparecían
3. **Sistema complejo** con muchas interdependencias

### Solución Final

Se creó una **versión simplificada y estable** que:
- ✅ Mantiene toda la funcionalidad
- ✅ Elimina reinicios 
- ✅ Usa LED debugging en lugar de Serial
- ✅ Arquitectura más robusta

El código en `src/main.cpp` es la **versión final estable** que debe usarse para producción.

---

**Nota**: Estos archivos se mantienen solo para referencia histórica y análisis. 
Para desarrollo usar siempre el código en `src/`.
