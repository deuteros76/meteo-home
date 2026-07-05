# Frontend de reconfiguración remota (Home Assistant) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Añadir un tópico MQTT retenido (`config/state`) donde el dispositivo publica su
configuración actual, y documentar una plantilla de Home Assistant (helpers + automatizaciones
+ script + tarjeta) que use ese estado para ofrecer un formulario de reconfiguración remota sin
componer JSON a mano.

**Architecture:** `Manager` gana un método puro `buildConfigStatePayload()` que serializa los 7
campos reconfigurables (sin token) a JSON con tipos nativos. `MeteoBoard::connectToMQTT()`
publica ese payload como retenido en `meteohome/<device>/config/state` en cada conexión, usando
streaming (`beginPublish`/`print`/`endPublish`) para no depender del tamaño de búfer de
`PubSubClient`. El lado de Home Assistant no toca el repo de firmware: se documenta como
plantilla YAML en el README.

**Tech Stack:** ESP8266/Arduino, ArduinoJson v6.21.x, PubSubClient, PlatformIO/Unity
(`d1_mini_lite`), Home Assistant (YAML: helpers, automations, scripts, Lovelace).

## Global Constraints

- Campos incluidos en `config/state`, en este orden: `device_name`, `use_sleep_mode`,
  `sleep_minutes`, `use_analog_sensor`, `sensor_class`, `use_arduino_map_function`,
  `analog_min_value`, `analog_max_value`. El campo `token`/`config_token` **nunca** se incluye
  en este payload.
- Tipos JSON nativos (no texto): booleanos como `true`/`false` JSON, enteros como números JSON
  — a diferencia de `persistConfig()`, que serializa todo a texto para `/config.json`.
- Tópico: `meteohome/<device_name>/config/state`, análogo a `configSetTopic()`/
  `configResultTopic()` ya existentes en `Manager`.
- Publicación **retenida** (`retain=true`), en cada conexión MQTT exitosa dentro de
  `MeteoBoard::connectToMQTT()`.
- La publicación debe usar `client->beginPublish()`/`print()`/`endPublish()` (streaming), **no**
  `client->publish()`: `MQTT_MAX_PACKET_SIZE` de `PubSubClient` es 256 bytes por defecto y no
  está sobrescrito en este proyecto; con `device_name` (máx. 40 car.) y `sensor_class` (máx. 20
  car.) cerca de sus límites, el payload puede acercarse o superar ese tamaño y `publish()`
  fallaría en silencio.
- Solo se prueba de forma automatizada la construcción del payload (`buildConfigStatePayload()`
  es una función pura). La publicación real por MQTT se verifica manualmente en hardware —
  mismo criterio que el resto del feature de reconfiguración remota (`native` sigue roto por el
  problema preexistente de `PubSubClient`/`ArduinoFake`, documentado en el spec original; se usa
  `d1_mini_lite` para todos los tests).
- Entorno de verificación de build/test en esta sesión: `pio run -e d1_mini_lite` (build) y
  `pio test -e d1_mini_lite --without-uploading --without-testing` (compilación de tests), salvo
  que haya hardware ESP8266 disponible y conectado.
- Fuera de alcance: cualquier cambio a `config/set`/`config/result`/`applyRemoteConfig`; una
  página HTML independiente; entidades MQTT discovery nativas por campo (descartadas en el
  diseño por riesgo de reinicios en cascada).

---

### Task 1: `Manager::buildConfigStatePayload()` y `configStateTopic()`

**Files:**
- Modify: `include/manager.hpp` (añadir getter `configStateTopic()` y declaración de
  `buildConfigStatePayload()`)
- Modify: `src/manager.cpp` (implementación de `buildConfigStatePayload()`)
- Modify: `test/tests/test_manager.cpp` (nuevo test)
- Modify: `test/test_run_all.cpp` (registrar el nuevo test)

**Interfaces:**
- Consumes: miembros privados existentes de `Manager` (`device_name`, `use_sleep_mode`,
  `sleep_minutes`, `use_analog_sensor`, `sensor_class`, `use_arduino_map_function`,
  `analog_min_value`, `analog_max_value`); `Manager::setDeviceName`, `setSleepMinutes`,
  `setUseSleepMode`, `setUseAnalogSensor`, `setSensorClass`, `setUseArduinoMapFunction`,
  `setAnalogMinValue`, `setAnalogMaxValue`, `persistConfigForTest()`, `setup_config_data()` (ya
  existen, ver `include/manager.hpp`).
- Produces: `String Manager::configStateTopic()` (pública, inline, igual patrón que
  `configSetTopic()`), `String Manager::buildConfigStatePayload()` (pública, declarada en el
  header junto a `applyRemoteConfig`, implementada en `manager.cpp`). Task 2 consume ambos.

- [ ] **Step 1: Escribir el test que falla**

  Abre `test/tests/test_manager.cpp`. Ya existe una función helper `makeConfiguredManager()`
  (línea ~123) que crea un `Manager` completamente configurado y con token generado — reutilízala.
  Añade este test al final del archivo:

  ```cpp
  void test_buildConfigStatePayloadExcludesTokenAndUsesNativeTypes() {
      Manager manager = makeConfiguredManager();

      String payload = manager.buildConfigStatePayload();

      DynamicJsonDocument doc(384);
      DeserializationError err = deserializeJson(doc, payload);
      TEST_ASSERT_FALSE(err);

      TEST_ASSERT_FALSE(doc.containsKey("token"));
      TEST_ASSERT_FALSE(doc.containsKey("config_token"));

      TEST_ASSERT_EQUAL_STRING("device_under_test", doc["device_name"].as<const char*>());
      TEST_ASSERT_TRUE(doc["use_sleep_mode"].is<bool>());
      TEST_ASSERT_FALSE(doc["use_sleep_mode"].as<bool>());
      TEST_ASSERT_TRUE(doc["sleep_minutes"].is<int>());
      TEST_ASSERT_EQUAL(5, doc["sleep_minutes"].as<int>());
      TEST_ASSERT_TRUE(doc["use_analog_sensor"].is<bool>());
      TEST_ASSERT_TRUE(doc["use_analog_sensor"].as<bool>());
      TEST_ASSERT_EQUAL_STRING("moisture", doc["sensor_class"].as<const char*>());
      TEST_ASSERT_TRUE(doc["use_arduino_map_function"].is<bool>());
      TEST_ASSERT_TRUE(doc["use_arduino_map_function"].as<bool>());
      TEST_ASSERT_EQUAL(0, doc["analog_min_value"].as<int>());
      TEST_ASSERT_EQUAL(1024, doc["analog_max_value"].as<int>());

      TEST_ASSERT_EQUAL(8, doc.size());
  }
  ```

  Registra el test en `test/test_run_all.cpp`, añadiendo esta línea justo después de
  `RUN_TEST(test_applyRemoteConfigEmptyDeviceName);`:

  ```cpp
  RUN_TEST(test_buildConfigStatePayloadExcludesTokenAndUsesNativeTypes);
  ```

- [ ] **Step 2: Verificar que compila y falla**

  `buildConfigStatePayload()` todavía no existe, así que esto debe fallar en compilación, no en
  ejecución. Ejecuta:

  ```bash
  pio test -e d1_mini_lite --without-uploading --without-testing
  ```

  Salida esperada: error de compilación tipo `'class Manager' has no member named
  'buildConfigStatePayload'`. Confirma que el error señala exactamente esa función antes de
  continuar.

- [ ] **Step 3: Implementar `configStateTopic()` y `buildConfigStatePayload()`**

  En `include/manager.hpp`, justo debajo de la línea 80
  (`String configResultTopic(){return "meteohome/" + device_name + "/config/result";}`), añade:

  ```cpp
  //! Topic where this device publishes its current configuration (retained), for
  //! external clients (e.g. Home Assistant) to read back before issuing a config/set command.
  String configStateTopic(){return "meteohome/" + device_name + "/config/state";}
  ```

  Justo debajo de la declaración de `applyRemoteConfig` (línea 102), añade:

  ```cpp
  //! Builds the JSON payload for config/state: the 7 remotely-configurable fields with
  //! native JSON types (not text), and no token.
  String buildConfigStatePayload();
  ```

  En `src/manager.cpp`, añade la implementación cerca de `applyRemoteConfig` (al final del
  archivo está bien):

  ```cpp
  String Manager::buildConfigStatePayload(){
    DynamicJsonDocument json(384);
    json["device_name"] = device_name;
    json["use_sleep_mode"] = use_sleep_mode;
    json["sleep_minutes"] = sleep_minutes;
    json["use_analog_sensor"] = use_analog_sensor;
    json["sensor_class"] = sensor_class;
    json["use_arduino_map_function"] = use_arduino_map_function;
    json["analog_min_value"] = analog_min_value;
    json["analog_max_value"] = analog_max_value;

    String payload;
    serializeJson(json, payload);
    return payload;
  }
  ```

- [ ] **Step 4: Verificar que el test pasa**

  ```bash
  pio test -e d1_mini_lite --without-uploading --without-testing
  ```

  Confirma que compila sin errores. Si hay hardware ESP8266 conectado, ejecuta también
  `pio test -e d1_mini_lite` y confirma que
  `test_buildConfigStatePayloadExcludesTokenAndUsesNativeTypes` pasa junto con el resto de la
  suite (sin warnings nuevos).

- [ ] **Step 5: Commit**

  ```bash
  git add include/manager.hpp src/manager.cpp test/tests/test_manager.cpp test/test_run_all.cpp
  git commit -m "feat: add Manager::buildConfigStatePayload for config/state topic"
  ```

---

### Task 2: Publicar `config/state` en cada conexión MQTT

**Files:**
- Modify: `src/meteoboard.cpp` (`connectToMQTT()`)

**Interfaces:**
- Consumes: `Manager::configStateTopic()`, `Manager::buildConfigStatePayload()` (Task 1),
  `PubSubClient::beginPublish(const char* topic, unsigned int plength, boolean retained)`,
  `PubSubClient::print(const String&)` (vía `Print::print`), `PubSubClient::endPublish()` —
  mismo patrón que `MeteoBoard::sendDiscoveryMessage()` en el mismo archivo (línea ~126).
- Produces: ningún símbolo nuevo consumido por otras tareas; este task cierra el flujo de
  firmware del plan.

- [ ] **Step 1: Localizar el punto de inserción**

  Abre `src/meteoboard.cpp` y localiza `MeteoBoard::connectToMQTT()` (línea ~70). El bloque
  relevante, dentro del `if (client->connect(...))`, termina así:

  ```cpp
    if (client->connect(clientName.c_str(), availabilityTopic.c_str(), 0, true, "offline")) {
      Serial.println("[Board] Connected to mqtt");
      client->publish(availabilityTopic.c_str(), "online", true);
      // Subscribe to Home Assistant birth topic to resend discovery on HASS restart
      client->subscribe("homeassistant/status");
      client->subscribe(manager->configSetTopic().c_str());
    } else {
  ```

- [ ] **Step 2: Añadir la publicación de estado**

  Sustituye ese bloque por (añadiendo las 6 líneas nuevas antes de `} else {`):

  ```cpp
    if (client->connect(clientName.c_str(), availabilityTopic.c_str(), 0, true, "offline")) {
      Serial.println("[Board] Connected to mqtt");
      client->publish(availabilityTopic.c_str(), "online", true);
      // Subscribe to Home Assistant birth topic to resend discovery on HASS restart
      client->subscribe("homeassistant/status");
      client->subscribe(manager->configSetTopic().c_str());

      // Publish current config as retained state, so external clients (e.g. Home
      // Assistant) can read it back before issuing a config/set command. Streamed via
      // beginPublish/print/endPublish (same pattern as sendDiscoveryMessage) because the
      // payload can approach PubSubClient's default 256-byte MQTT_MAX_PACKET_SIZE with a
      // long device_name/sensor_class, and publish() would silently fail past that limit.
      String stateTopic = manager->configStateTopic();
      String statePayload = manager->buildConfigStatePayload();
      if (client->beginPublish(stateTopic.c_str(), statePayload.length(), true)) {
        client->print(statePayload);
        client->endPublish();
      } else {
        Serial.println("[Board] Error publishing config/state");
      }
    } else {
  ```

- [ ] **Step 3: Verificar que compila**

  ```bash
  pio run -e d1_mini_lite
  ```

  Expected: `SUCCESS`.

  ```bash
  pio test -e d1_mini_lite --without-uploading --without-testing
  ```

  Expected: compila sin errores (no hay test automatizado nuevo para este paso — la
  publicación MQTT real se verifica manualmente en hardware, ver Step 4).

- [ ] **Step 4: Verificación manual en hardware (si hay un dispositivo disponible)**

  Con un ESP8266 flasheado y conectado al broker, suscríbete a
  `meteohome/<device>/config/state` (usa el `device_name` real del dispositivo, p. ej.
  `attic`):

  ```bash
  mosquitto_sub -h <broker> -t 'meteohome/attic/config/state' -v
  ```

  Al conectar el dispositivo (o al despertar de deep-sleep), debe aparecer un mensaje JSON con
  los 7 campos y **sin** `token`. Si no hay hardware disponible en esta sesión, deja constancia
  en el reporte de que este paso queda pendiente de verificación manual, igual que se hizo para
  el resto del feature de reconfiguración remota.

- [ ] **Step 5: Commit**

  ```bash
  git add src/meteoboard.cpp
  git commit -m "feat: publish retained config/state on every MQTT connection"
  ```

---

### Task 3: Documentar `config/state` y la plantilla de Home Assistant en el README

**Files:**
- Modify: `README.md` (sección "Reconfiguración remota", líneas ~128-162 en el estado actual)

**Interfaces:**
- Consumes: `config/state` (Task 2), `config/set`/`config/result` (ya documentados), los
  nombres de campo del Global Constraints de este plan.
- Produces: ninguno (tarea de documentación, no de código).

- [ ] **Step 1: Añadir la descripción del tópico `config/state`**

  En `README.md`, justo antes de la sección `## Using MeteoHome with Home Assistant` (después
  del párrafo que termina en "...para no reprocesarlo en la siguiente reconexión."), añade:

  ```markdown
  Además, en cada conexión al broker el dispositivo publica su configuración actual (sin el
  token) como retenido en `meteohome/<device>/config/state`:

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

  Este tópico permite construir un formulario (por ejemplo, en Home Assistant) que se
  autorrellena con los valores reales del dispositivo antes de enviar un cambio.

  ### Formulario en Home Assistant

  Con `config/state` y `config/set`/`config/result` se puede montar un formulario en Home
  Assistant que lee el estado real del dispositivo y aplica cambios con un botón, sin
  componer JSON a mano. Por cada dispositivo (usando su `device_name` como identificador,
  p. ej. `attic`), añade estos helpers en Ajustes → Dispositivos y servicios → Ayudantes, o
  directamente en `configuration.yaml`:

  ```yaml
  input_text:
    meteohome_attic_device_name:
      name: "Attic - nombre"
    meteohome_attic_sensor_class:
      name: "Attic - clase de sensor"
    meteohome_attic_token:
      name: "Attic - token"
      mode: password

  input_boolean:
    meteohome_attic_use_sleep_mode:
      name: "Attic - modo sleep"
    meteohome_attic_use_analog_sensor:
      name: "Attic - sensor analógico"
    meteohome_attic_use_arduino_map_function:
      name: "Attic - usar map()"

  input_number:
    meteohome_attic_sleep_minutes:
      name: "Attic - minutos de sleep"
      min: 1
      max: 60
      step: 1
    meteohome_attic_analog_min_value:
      name: "Attic - valor mínimo analógico"
      min: 0
      max: 1024
    meteohome_attic_analog_max_value:
      name: "Attic - valor máximo analógico"
      min: 0
      max: 1024
  ```

  Automatización que mantiene los helpers sincronizados con el estado real del dispositivo:

  ```yaml
  automation:
    - alias: "MeteoHome attic - sincronizar estado"
      trigger:
        - platform: mqtt
          topic: "meteohome/attic/config/state"
      action:
        - service: input_text.set_value
          target: {entity_id: input_text.meteohome_attic_device_name}
          data: {value: "{{ trigger.payload_json.device_name }}"}
        - service: "input_boolean.turn_{{ 'on' if trigger.payload_json.use_sleep_mode else 'off' }}"
          target: {entity_id: input_boolean.meteohome_attic_use_sleep_mode}
        - service: input_number.set_value
          target: {entity_id: input_number.meteohome_attic_sleep_minutes}
          data: {value: "{{ trigger.payload_json.sleep_minutes }}"}
        - service: "input_boolean.turn_{{ 'on' if trigger.payload_json.use_analog_sensor else 'off' }}"
          target: {entity_id: input_boolean.meteohome_attic_use_analog_sensor}
        - service: input_text.set_value
          target: {entity_id: input_text.meteohome_attic_sensor_class}
          data: {value: "{{ trigger.payload_json.sensor_class }}"}
        - service: "input_boolean.turn_{{ 'on' if trigger.payload_json.use_arduino_map_function else 'off' }}"
          target: {entity_id: input_boolean.meteohome_attic_use_arduino_map_function}
        - service: input_number.set_value
          target: {entity_id: input_number.meteohome_attic_analog_min_value}
          data: {value: "{{ trigger.payload_json.analog_min_value }}"}
        - service: input_number.set_value
          target: {entity_id: input_number.meteohome_attic_analog_max_value}
          data: {value: "{{ trigger.payload_json.analog_max_value }}"}
  ```

  Script que aplica los cambios (arma el JSON con los valores actuales de los helpers y lo
  publica en `config/set`):

  ```yaml
  script:
    meteohome_attic_apply_config:
      alias: "MeteoHome attic - aplicar configuración"
      sequence:
        - service: mqtt.publish
          data:
            topic: "meteohome/attic/config/set"
            retain: true
            payload: >
              {{ {
                "token": states('input_text.meteohome_attic_token'),
                "device_name": states('input_text.meteohome_attic_device_name'),
                "use_sleep_mode": is_state('input_boolean.meteohome_attic_use_sleep_mode', 'on'),
                "sleep_minutes": states('input_number.meteohome_attic_sleep_minutes') | int,
                "use_analog_sensor": is_state('input_boolean.meteohome_attic_use_analog_sensor', 'on'),
                "sensor_class": states('input_text.meteohome_attic_sensor_class'),
                "use_arduino_map_function": is_state('input_boolean.meteohome_attic_use_arduino_map_function', 'on'),
                "analog_min_value": states('input_number.meteohome_attic_analog_min_value') | int,
                "analog_max_value": states('input_number.meteohome_attic_analog_max_value') | int
              } | tojson }}
  ```

  Tarjeta Lovelace con el formulario y el botón de aplicar:

  ```yaml
  type: entities
  title: MeteoHome - attic
  entities:
    - input_text.meteohome_attic_device_name
    - input_boolean.meteohome_attic_use_sleep_mode
    - input_number.meteohome_attic_sleep_minutes
    - input_boolean.meteohome_attic_use_analog_sensor
    - input_text.meteohome_attic_sensor_class
    - input_boolean.meteohome_attic_use_arduino_map_function
    - input_number.meteohome_attic_analog_min_value
    - input_number.meteohome_attic_analog_max_value
    - input_text.meteohome_attic_token
    - entity: script.meteohome_attic_apply_config
      name: "Aplicar cambios"
  ```

  Y una automatización que notifica el resultado en la propia UI de Home Assistant:

  ```yaml
  automation:
    - alias: "MeteoHome attic - notificar resultado"
      trigger:
        - platform: mqtt
          topic: "meteohome/attic/config/result"
      action:
        - service: persistent_notification.create
          data:
            title: "MeteoHome attic"
            message: >
              {{ 'Configuración aplicada: ' ~ trigger.payload_json.fields | join(', ')
                 if trigger.payload_json.status == 'applied'
                 else 'Error: ' ~ trigger.payload_json.reason }}
  ```

  **Notas:**
  - Repite esta plantilla por cada dispositivo, cambiando `attic` por el `device_name` real en
    los IDs de entidad y en los tópicos.
  - Si renombras un dispositivo cambiando `device_name` desde este formulario, el dispositivo
    pasa a publicar todo (estado, resultado, sensores, disponibilidad) bajo el nuevo nombre a
    partir del siguiente reinicio; actualiza el "slug" en la plantilla YAML de este dispositivo
    a mano tras el cambio.
  - `mode: password` en el helper del token solo lo oculta en la interfaz; Home Assistant lo
    guarda igual que cualquier otro estado, con el mismo nivel de confianza que ya tienen las
    credenciales MQTT de tu propio `configuration.yaml`.
  - Si tu broker no persiste mensajes retenidos en disco (p. ej. Mosquitto con
    `persistence false`), un reinicio del broker hace que se pierda el último `config/state`
    hasta que el dispositivo se reconecte en su siguiente ciclo (más notable con intervalos de
    deep-sleep largos).
  ```

- [ ] **Step 2: Revisión visual**

  Renderiza el README (o ábrelo en un visor de Markdown) y confirma que los bloques de código
  YAML/JSON no rompen el formato de las secciones adyacentes, y que el orden de secciones queda:
  "Reconfiguración remota" (con `config/set`, `config/result`, `config/state` y el formulario de
  HA) seguida de "Using MeteoHome with Home Assistant".

- [ ] **Step 3: Commit**

  ```bash
  git add README.md
  git commit -m "docs: document config/state topic and Home Assistant remote-config template"
  ```
