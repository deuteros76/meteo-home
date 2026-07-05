# Diseño: reconfiguración remota segura vía MQTT

**Fecha:** 2026-07-05
**Estado:** Aprobado para planificación

## Problema

Hoy la única forma de reconfigurar un dispositivo MeteoHome ya desplegado es borrar
manualmente `/config.json` (acceso físico) para forzar que vuelva a arrancar el portal
cautivo de `WiFiManager`. No existe ninguna vía para cambiar parámetros del dispositivo sin
manipularlo físicamente.

## Alcance

Se añade un canal de reconfiguración remota vía MQTT, limitado a los parámetros de
dispositivo/sensores que **no** afectan a la conectividad de red:

- `device_name`
- `use_sleep_mode`
- `sleep_minutes`
- `use_analog_sensor`
- `sensor_class`
- `use_arduino_map_function`
- `analog_min_value`
- `analog_max_value`

Explícitamente **fuera de alcance**: red WiFi, servidor/usuario/contraseña MQTT y el propio
`config_token` no son reconfigurables por este canal. Un cambio incorrecto en esos campos
podría dejar el dispositivo inalcanzable, y seguirían requiriendo el borrado manual de
`config.json` como hoy. Rotar el token también requiere ese mismo camino (borrar
`config.json` y volver a pasar por el portal).

## Arquitectura

### Componentes modificados

**`Manager`** (`manager.{hpp,cpp}`)
- Nuevo miembro `config_token` (String), persistido en `/config.json`.
- Nuevo getter `configToken()`.
- Nuevos setters privados/internos para los ocho campos remotamente configurables listados
  arriba.
- Nuevo método `bool applyRemoteConfig(JsonDocument &doc, String &errorReason)`: valida token
  y campos, actualiza los miembros correspondientes y persiste. Devuelve `false` sin mutar
  estado si algo falla.
- Nuevo método privado `persistConfig()`: serializa **todos** los miembros actuales
  (incluido `config_token`) a `/config.json`. Sustituye al bloque de guardado actual en
  `setup_wifi()`, que hoy serializa directamente desde los `WiFiManagerParameter` en lugar de
  desde los miembros de la clase — se refactoriza para que el bloque de guardado del portal
  asigne primero a los miembros y después llame a `persistConfig()`, evitando duplicar la
  lógica de serialización entre el flujo del portal y el flujo remoto.
- Generación del token: si `config_token` no existe en `/config.json` (dispositivo nuevo o
  actualización de firmware sobre un dispositivo ya configurado), se genera con
  `secureRandom()` (RNG hardware del núcleo ESP8266, sin dependencias nuevas): 16 bytes
  aleatorios codificados en hex (32 caracteres), y se persiste inmediatamente.

**`MeteoBoard`** (`meteoboard.{hpp,cpp}`)
- `connectToMQTT()` añade una suscripción a `meteohome/<device>/config/set`, junto a la ya
  existente `homeassistant/status`.
- `mqttCallback` distingue el tópico de config del de `homeassistant/status` y delega en un
  nuevo método `handleConfigCommand(String payload)`.
- `handleConfigCommand()`:
  1. Parsea el JSON del payload.
  2. Llama a `manager->applyRemoteConfig(doc, errorReason)`.
  3. Publica el resultado en `meteohome/<device>/config/result` (no retenido).
  4. Limpia el comando publicando un mensaje vacío retenido en `config/set` (para no
     reprocesarlo en la siguiente reconexión/arranque).
  5. Si se aplicó correctamente, llama a `ESP.restart()` (mismo patrón que el guardado desde
     el portal cautivo en `Manager::setup_wifi()`).

**Portal cautivo (`manager.cpp`, `setup_wifi()`)**
- Cuando el portal se ejecuta por primera vez (dispositivo nuevo, sin `config.json`), se
  añade un `WiFiManagerParameter` de solo lectura que muestra el `config_token` recién
  generado, con una nota indicando que hay que guardar ese valor para poder reconfigurar el
  dispositivo remotamente más adelante.
- En dispositivos ya configurados que actualizan de firmware, el portal no se ejecuta (ya
  existe `config.json`), así que el token generado en ese caso solo se imprime una vez por
  Serial al arrancar. Se documentará en el README: revisar el monitor serie tras la primera
  actualización de firmware, o borrar `config.json` para volver a pasar por el portal y
  verlo ahí.

### Flujo de datos

1. Un cliente externo (Home Assistant, script, `mosquitto_pub`) publica un JSON con
   `retain=true` en `meteohome/<device>/config/set`, incluyendo el token y los campos a
   cambiar.
2. Al conectar o reconectar (incluido el ciclo breve de conexión en modo deep-sleep), el
   dispositivo recibe el mensaje retenido vía `mqttCallback`.
3. `MeteoBoard::handleConfigCommand()` procesa el payload como se describe arriba.
4. Si el token y los valores son válidos, `Manager` actualiza sus miembros y persiste; el
   dispositivo reinicia para que todo lo derivado (tópicos derivados del nombre, hostname
   OTA, temporizadores de sleep, etc.) se reinicialice de forma consistente, igual que ocurre
   hoy tras guardar desde el portal.

## Formato del payload

Actualización parcial: solo se modifican los campos presentes en el payload. `token` es
obligatorio; el resto son opcionales.

```json
{
  "token": "a1b2c3...",
  "device_name": "Attic",
  "use_sleep_mode": true,
  "sleep_minutes": 15,
  "use_analog_sensor": false,
  "sensor_class": "moisture",
  "use_arduino_map_function": true,
  "analog_min_value": 0,
  "analog_max_value": 1024
}
```

Campos no reconocidos en el payload se ignoran (se registran por Serial, no interrumpen el
procesamiento del resto).

## Seguridad

- **Autenticidad:** el bróker MQTT ya exige usuario/contraseña (capa existente). Además, todo
  comando de reconfiguración debe incluir el `config_token` del dispositivo, generado
  aleatoriamente y no editable por el usuario ni por este mismo canal. Esto evita que
  cualquier cliente autenticado en el bróker (que puede ser compartido con otros
  dispositivos/integraciones) reconfigure un MeteoHome sin conocer su token específico.
- **Comparación de token:** comparación de cadena estándar (`token == config_token`). No se
  usa comparación en tiempo constante: el canal va sobre una conexión MQTT ya autenticada por
  TCP, y no es un vector de temporización práctico en este contexto; añadir eso sería
  sobre-ingeniería para este caso de uso.
- **Superficie de ataque limitada:** el canal solo permite modificar parámetros que no
  afectan a la conectividad (ver "Alcance"). Un comando erróneo o malicioso en el peor caso
  cambia el nombre del dispositivo o el comportamiento de un sensor, nunca deja el
  dispositivo inalcanzable.
- **Sin rotación remota:** el token no puede cambiarse por este canal; rotarlo exige acceso
  físico (borrar `config.json` y repetir el portal), igual que cualquier otra
  reconfiguración de red hoy.

## Manejo de errores

Todas las validaciones son transaccionales: si un solo campo del payload falla, se rechaza
el comando completo y no se aplica ningún cambio.

| Situación | Respuesta en `config/result` | ¿Reinicia? |
|---|---|---|
| Token ausente o incorrecto | `{"status":"error","reason":"invalid_token"}` | No |
| JSON malformado | `{"status":"error","reason":"invalid_payload"}` | No |
| Campo fuera de rango/tipo inválido | `{"status":"error","reason":"invalid_field:<nombre>"}` | No |
| Todo válido | `{"status":"applied","fields":[...]}` | Sí |

Validaciones por campo:
- `sleep_minutes`: entero 1–60.
- `analog_min_value` / `analog_max_value`: enteros 0–1024, con `min < max`.
- `sensor_class` / `device_name`: no vacíos.
- Campos booleanos: deben deserializar como booleano válido.

En todos los casos (éxito o error) se limpia el tópico `config/set` publicando un mensaje
vacío retenido, para no reprocesar el mismo comando en la siguiente reconexión.

## Testing

- `native` (ArduinoFake): tests unitarios para `Manager::applyRemoteConfig()` cubriendo token
  correcto/incorrecto/ausente, cada validación de campo, actualización parcial (solo algunos
  campos presentes), y que `persistConfig()` serializa correctamente todos los miembros
  (incluido `config_token`).
- `native`: tests para `MeteoBoard::handleConfigCommand()` (mockeando `PubSubClient`)
  verificando que se publica el resultado correcto en `config/result` y que se limpia el
  retained de `config/set` en cada caso (éxito y error).
- La generación del token vía `secureRandom()` no se testea en `native` por depender de RNG
  hardware; se verifica manualmente en `d1_mini_lite` (token visible en el portal en un
  dispositivo nuevo, y por Serial en una actualización de firmware sobre un dispositivo ya
  configurado).

## Documentación

Se actualiza `README.md` con: la nueva sección de "Configuración remota", los tópicos
`config/set` / `config/result`, el formato del payload, y la nota sobre dónde encontrar el
`config_token` en cada escenario (portal para dispositivos nuevos, Serial para
actualizaciones de firmware existentes).
