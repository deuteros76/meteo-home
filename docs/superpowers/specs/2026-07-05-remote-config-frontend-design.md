# Diseño: frontend de reconfiguración remota vía Home Assistant

**Fecha:** 2026-07-05
**Estado:** Aprobado para planificación

## Problema

La funcionalidad de reconfiguración remota vía MQTT (`meteohome/<device>/config/set`,
ver [`2026-07-05-remote-config-design.md`](2026-07-05-remote-config-design.md)) solo se puede
usar hoy publicando manualmente un JSON con `mosquitto_pub`, incluyendo el token de memoria y
recordando qué campos existen y qué formato tienen. No hay una forma cómoda de reconfigurar un
dispositivo sin componer el JSON a mano, y no hay manera de conocer los valores actuales del
dispositivo antes de cambiarlos: el dispositivo no expone su configuración por MQTT, solo se
ve borrando `config.json` o en el portal cautivo.

## Alcance

Se añaden dos piezas:

1. **Firmware**: un nuevo tópico retenido `meteohome/<device>/config/state`, publicado por el
   dispositivo en cada conexión MQTT, con su configuración actual (los mismos 7 campos
   reconfigurables, **sin el token**).
2. **Home Assistant**: una plantilla YAML documentada (helpers + automatizaciones + script +
   tarjeta Lovelace) por dispositivo, que sincroniza un formulario con el estado real del
   dispositivo y permite aplicar cambios con un botón, sin escribir JSON a mano.

Explícitamente **fuera de alcance**:
- Una página HTML independiente o cualquier frontend fuera de Home Assistant (se descartó en
  el brainstorming: el usuario ya usa HA como panel principal y gestiona pocos dispositivos de
  forma ocasional, por lo que no compensa mantener una interfaz nueva y separada).
- Entidades nativas de HA vía MQTT discovery por campo (se descartó: el firmware reinicia el
  dispositivo tras cualquier cambio aplicado con éxito, y una entidad nativa por campo
  arriesgaría reinicios en cascada por cada ajuste individual —p. ej. al arrastrar un slider—
  en vez de un envío atómico de todos los campos a la vez).
- Cualquier cambio al protocolo `config/set` / `config/result` ya existente: se reutiliza tal
  cual.
- Rotación o visualización del `config_token` por este canal: sigue exclusivamente disponible
  por el portal cautivo o Serial, como ya establece el diseño original.

## Arquitectura

### Componentes modificados (firmware)

**`Manager`** (`manager.{hpp,cpp}`)
- Nuevo getter `configStateTopic()`: `"meteohome/" + device_name + "/config/state"`.
- Nuevo método `String buildConfigStatePayload()`: serializa los 7 campos reconfigurables
  (`device_name`, `use_sleep_mode`, `sleep_minutes`, `use_analog_sensor`, `sensor_class`,
  `use_arduino_map_function`, `analog_min_value`, `analog_max_value`) a JSON, **sin** el
  `config_token`. A diferencia de `persistConfig()` (que serializa todo como texto para
  compatibilidad con el parseo existente de `/config.json`), aquí se usan los tipos JSON
  nativos (booleanos y enteros reales), de forma que el payload de `config/state` sea
  directamente reutilizable como payload de `config/set` (añadiendo el token) sin conversión.

**`MeteoBoard`** (`meteoboard.cpp`)
- En `connectToMQTT()`, tras las suscripciones existentes, se publica el estado usando el
  patrón de streaming ya usado por `sendDiscoveryMessage()` (`beginPublish()` / `print()` /
  `endPublish()`), no `client->publish()` directo:

  ```cpp
  String stateTopic = manager->configStateTopic();
  String statePayload = manager->buildConfigStatePayload();
  if (client->beginPublish(stateTopic.c_str(), statePayload.length(), true)) {
    client->print(statePayload);
    client->endPublish();
  }
  ```

  **Motivo de usar streaming en vez de `publish()`:** `MQTT_MAX_PACKET_SIZE` de `PubSubClient`
  es 256 bytes por defecto (topic + payload + cabecera) y no está sobrescrito en este proyecto.
  Con un `device_name` largo (hasta 40 caracteres, máximo permitido por el portal) y
  `sensor_class` cerca de su máximo (20 caracteres), el payload de estado puede acercarse o
  superar ese límite, y `publish()` fallaría en silencio. El streaming de `beginPublish`/
  `print`/`endPublish` no depende del tamaño del búfer, evitando el problema sin tener que
  aumentar `MQTT_MAX_PACKET_SIZE`.
- Se publica en cada conexión MQTT exitosa (incluido cada ciclo de reconexión en modo
  deep-sleep), como retenido, para que el último estado conocido esté siempre disponible al
  suscribirse (incluido tras un reinicio de Home Assistant).

### Componente nuevo (Home Assistant, documentado en README, no en el repo de firmware)

Por cada dispositivo (usando su `device_name` como "slug", p. ej. `attic`):

- **Helpers**: `input_text` (nombre, clase de sensor, token), `input_boolean` (los 3 flags),
  `input_number` (minutos de sleep, valores mínimo/máximo analógicos) — representan el
  formulario editable.
- **Automatización de sincronización**: trigger MQTT sobre `meteohome/<device>/config/state`,
  que actualiza cada helper con el valor recibido (`input_text.set_value`,
  `input_number.set_value`, `input_boolean.turn_on`/`turn_off` según el campo booleano
  correspondiente).
- **Script "Aplicar"**: construye el JSON completo (token + 7 campos) a partir del valor
  actual de los helpers y lo publica con `mqtt.publish` (`retain: true`) en
  `meteohome/<device>/config/set`.
- **Tarjeta Lovelace**: tarjeta de entidades con los 8 campos del formulario y un botón que
  ejecuta el script.
- **Automatización de notificación de resultado**: trigger MQTT sobre
  `meteohome/<device>/config/result`, que crea una `persistent_notification` con el resultado
  (campos aplicados, o motivo del error), para que el éxito o fallo de la reconfiguración sea
  visible en la propia UI de HA sin inspeccionar MQTT directamente.

### Flujo de datos

```
Dispositivo conecta MQTT → publica config/state (retenido, sin token)
                                    │
                                    ▼
HA: automatización de sincronización → actualiza helpers con valores reales
                                    │
                                    ▼
Usuario edita helpers en la tarjeta Lovelace → pulsa "Aplicar"
                                    │
                                    ▼
HA: script → mqtt.publish a config/set (token + 7 campos, retain=true)
                                    │
                                    ▼
Dispositivo aplica cambio (o rechaza) → publica config/result
                                    │
                                    ▼
HA: automatización de notificación → persistent_notification con el resultado
                                    │
                                    ▼
Si se aplicó, el dispositivo se reinicia y, al reconectar, vuelve a publicar
config/state → HA resincroniza los helpers con el nuevo estado real
```

## Formato del payload de `config/state`

```json
{
  "device_name": "attic",
  "use_sleep_mode": true,
  "sleep_minutes": 15,
  "use_analog_sensor": false,
  "sensor_class": "moisture",
  "use_arduino_map_function": true,
  "analog_min_value": 0,
  "analog_max_value": 1024
}
```

Mismo conjunto de campos que `config/set`, sin `token`, con tipos JSON nativos (no texto).

## Seguridad

- **Token no incluido en `config/state`**: publicar el token en un tópico retenido de solo
  lectura lo expondría de forma pasiva a cualquier suscriptor sin que se emita ningún comando,
  aumentando su superficie de exposición respecto al diseño original (donde el token solo
  viaja cuando alguien lo usa activamente para reconfigurar). El usuario ya conoce su token por
  el portal/Serial, y el formulario de HA lo guarda en un `input_text` aparte, solo para
  construir el payload de `config/set`.
- **Token en Home Assistant**: `mode: password` en el `input_text` del token solo oculta el
  valor en la UI; HA lo guarda igualmente en claro en su estado interno, igual que ya ocurre
  con las credenciales MQTT del propio `configuration.yaml`. Es el mismo nivel de confianza que
  ya se asume al dar a HA acceso al broker; no es una relajación de seguridad respecto al
  diseño original del token.
- No se introduce ningún cambio en la validación, autenticación o superficie de ataque del
  canal `config/set` ya existente.

## Casos límite y comportamiento esperado

- **Renombrar un dispositivo** (`device_name`): al ser parte del tópico
  (`meteohome/<device_name>/...`), cambiarlo hace que el dispositivo publique todo bajo el
  nuevo nombre a partir del siguiente reinicio. Los helpers/automatizaciones/script de ese
  dispositivo quedan apuntando al tópico antiguo; hay que actualizar manualmente el "slug" en
  la plantilla YAML. Se documentará como nota junto a la plantilla.
- **Broker sin persistencia de mensajes retenidos**: si el broker se reinicia sin persistencia
  en disco, se pierde el último `config/state` hasta que el dispositivo se reconecte en su
  siguiente ciclo (más relevante con intervalos de deep-sleep largos). No requiere manejo
  especial.
- **`config/result` de error sin campo `fields`**: la plantilla Jinja de la notificación de
  resultado no accede a `fields` en la rama de error, solo a `reason`.
- **Varios dispositivos**: la plantilla se repite cambiando el "slug" en IDs de entidad y
  tópicos, igual que ya ocurre con las tarjetas de sensores existentes documentadas para HA.

## Testing

- **Firmware**: test unitario en `test_manager.cpp` para `buildConfigStatePayload()` — JSON
  válido, los 7 campos con tipo correcto (booleanos como `bool`, no como texto), y ausencia de
  la clave `token`. Es una función pura sin dependencia de `PubSubClient`, se ejecuta en
  `d1_mini_lite` (mismo entorno usado en el resto de tests de esta área; `native` sigue roto
  por el problema preexistente de `PubSubClient`/`ArduinoFake` documentado en el spec
  original).
- La publicación real vía streaming en `connectToMQTT()` se verifica manualmente en hardware
  (mensaje retenido visible al suscribirse a `config/state`), no hay infraestructura de test
  para esa parte.
- **Home Assistant**: la plantilla YAML no es código del repo ni tiene test automatizado; se
  verifica manualmente (aplicar un cambio desde la tarjeta, comprobar la notificación de
  resultado, comprobar que los helpers se resincronizan tras un reinicio del dispositivo o de
  Home Assistant).

## Documentación

Se actualiza `README.md`, ampliando la sección "Reconfiguración remota" con: el nuevo tópico
`config/state`, su formato, y la plantilla YAML completa de Home Assistant (helpers,
automatizaciones, script y tarjeta) con las notas sobre renombrado de dispositivos y
persistencia de retenidos.
