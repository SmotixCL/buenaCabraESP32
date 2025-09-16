# Bitácora creada por Ricardo para agregar los cambios y avances desde el 16.09.2025

## 16.09.2025

- Se resolvió el problema de que llegaba la geocerca poligonal comprimida, pero no era seteada
  -> Simplemente faltaba un caso para geocercas tipo 2 en onGeofenceUpdate de main.cpp

- Se removió toda la lógica de nievels de alertas que estaba presente en GeofenceManager. La idea es que
  AlertManager maneje todo lo referente a alertas, y solo se apoye en GeofenceManager para obtener la distancia.

- El flujo para verificar posición con respecto a geocerca es:
  main.cpp -> en el loop, si existe geocerca configurada y gps fix, cada 10s extrae la distancia usando geofenceManager.getDistance
  y llama a alertManager.update con esta distancia.

AlertManager.cpp -> en el update(float distance) calculamos el nuevo nivel, y llamamos setAlertLevel en base a este.
-> setAlertLevel maneja iniciar la alerta llamando a buzzerManager.startContinousAlert(), detenerla con buzzerManager.stopContinousAlert(), y en cada ejecución, si hay nivel de alerta llama executeAlert()
-> executeAlert, si las alertas de audio están permitidas (que por defecto lo están) hace un log y llama updateBuzzer(), que es un método interno de AlertManager que llama a buzzerManager.updateContinousAlert() si la alerta continua está iniciada.

BuzzerManager.cpp -> updateContinousAlert toca el tono que corresponda, en base a las configs

- POR RESOLVER: ¿Realmente el tono que suena está haciendo caso a las configs que existen para cada nivel de alerta, o está mal la implementación en BuzzerManager.cpp?
