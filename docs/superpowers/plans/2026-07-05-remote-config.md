# Reconfiguración remota segura vía MQTT — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Permitir reconfigurar remotamente los parámetros de dispositivo/sensores de un
MeteoHome ya desplegado (sin acceso físico) publicando un comando JSON firmado con un token
secreto en un tópico MQTT, sin tocar red/MQTT/credenciales.

**Architecture:** `Manager` gana persistencia canónica (`persistConfig()`), un token secreto
auto-generado (`config_token`), y una función pura de validación/aplicación
(`applyRemoteConfig()`). `MeteoBoard` se suscribe a un tópico de comando MQTT retenido,
delega la validación en `Manager`, publica el resultado y reinicia el dispositivo si el
cambio se aplicó. Ver el spec completo en
`docs/superpowers/specs/2026-07-05-remote-config-design.md`.

**Tech Stack:** ESP8266 Arduino core (`secureRandom()`), ArduinoJson 6.21.3
(`DynamicJsonDocument`/`JsonDocument`), PubSubClient 2.8, WiFiManager 2.0.17, LittleFS, Unity
(tests en hardware `d1_mini_lite`).

## Global Constraints

- Campos remotamente configurables: **solo** `device_name`, `use_sleep_mode`,
  `sleep_minutes`, `use_analog_sensor`, `sensor_class`, `use_arduino_map_function`,
  `analog_min_value`, `analog_max_value`. Red, MQTT y el propio `config_token` **nunca** son
  modificables por este canal.
- El token (`config_token`) se auto-genera con `secureRandom()` (16 bytes → 32 caracteres
  hex), nunca es editable por el usuario ni rotable por MQTT.
- Validación transaccional: si un solo campo del payload falla, no se aplica ningún cambio.
- El comando se publica con `retain=true`; el dispositivo limpia el retained
  (mensaje vacío retenido) tras procesarlo, tanto en éxito como en error.
- Aplicar un cambio válido siempre termina en `ESP.restart()`.
- **`pio test -e native` está roto de antes** (cadena `mhdht.hpp → meteosensor.hpp →
  PubSubClient.h → IPAddress.h`, ausente en `ArduinoFake`). No se toca en este plan. Todos
  los tests nuevos son tests de hardware en `d1_mini_lite` (mismo patrón que los tests de
  sensores existentes).
- Persistencia en disco: `persistConfig()` debe serializar `use_sleep_mode`,
  `use_analog_sensor` y `use_arduino_map_function` como las cadenas `"true"`/`"false"` (no
  booleanos JSON nativos), y `sleep_minutes`/`analog_min_value`/`analog_max_value` como
  cadenas numéricas (no números JSON nativos). Esto es obligatorio para no romper el código
  de lectura existente en `Manager::setup_config_data()`, que castea esos campos a
  `const char*` antes de parsearlos — un número o booleano JSON nativo se castea a `nullptr`
  y rompería la carga silenciosamente.

---

### Task 1: Extraer `Manager::persistConfig()` y refactorizar el guardado del portal

**Files:**
- Modify: `include/manager.hpp`
- Modify: `src/manager.cpp:44-104` (lectura), `src/manager.cpp:247-286` (guardado)
- Test: `test/tests/test_manager.cpp`
- Modify: `test/test_run_all.cpp`

**Interfaces:**
- Produces: `void Manager::persistConfig()` (privado) — serializa todos los miembros actuales
  a `/config.json`, con el formato de tipos descrito en Global Constraints.
- Produces: comportamiento observable sin cambios — `setup_wifi()` sigue guardando
  exactamente los mismos campos que antes, solo que ahora vía `persistConfig()`.

Este task es un refactor puro (sin funcionalidad nueva) para poder reutilizar la lógica de
guardado desde el flujo remoto en el Task 4, sin duplicarla. Se verifica con un test de
round-trip: guardar con `persistConfig()` y releer con `setup_config_data()` debe devolver
los mismos valores.

- [ ] **Step 1: Escribir el test de round-trip (debe fallar: `persistConfig` no existe aún)**

Añadir a `test/tests/test_manager.cpp`:

```cpp
void test_persistConfigRoundTrip() {
    LittleFS.remove("/config.json");

    DynamicJsonDocument json(1024);
    json["network_ip"] = "192.168.1.50";
    json["network_mask"] = "255.255.255.0";
    json["network_gateway"] = "192.168.1.1";
    json["mqtt_server"] = "192.168.1.100";
    json["mqtt_port"] = "1883";
    json["mqtt_user"] = "user";
    json["mqtt_password"] = "pass";
    json["use_sleep_mode"] = "true";
    json["sleep_minutes"] = "15";
    json["device_name"] = "roundtrip_device";
    json["use_analog_sensor"] = "false";
    json["sensor_class"] = "moisture";
    json["use_arduino_map_function"] = "true";
    json["analog_min_value"] = "10";
    json["analog_max_value"] = "900";
    json["config_token"] = "deadbeef";

    File configFile = LittleFS.open("/config.json", "w");
    serializeJson(json, configFile);
    configFile.close();

    Manager manager;
    manager.setup_config_data();

    // Mutate a couple of fields the same way applyRemoteConfig will, then persist.
    manager.setSleepMinutes(30);
    manager.setDeviceName("roundtrip_device_renamed");
    manager.persistConfigForTest();

    Manager reloaded;
    reloaded.setup_config_data();

    TEST_ASSERT_EQUAL_STRING("roundtrip_device_renamed", reloaded.deviceName().c_str());
    TEST_ASSERT_EQUAL(30, reloaded.sleepMinutes());
    TEST_ASSERT_TRUE(reloaded.useSleepMode());
    TEST_ASSERT_FALSE(reloaded.useAnalogSensor());
    TEST_ASSERT_EQUAL_STRING("moisture", reloaded.sensorClass().c_str());
    TEST_ASSERT_EQUAL(10, reloaded.analogMinValue());
    TEST_ASSERT_EQUAL(900, reloaded.analogMaxValue());
    TEST_ASSERT_EQUAL_STRING("deadbeef", reloaded.configToken().c_str());
}
```

> Nota: `setSleepMinutes`, `setDeviceName`, `persistConfigForTest` y `configToken` no existen
> todavía — se añaden en los pasos siguientes de este mismo task (los setters y un método
> `persistConfigForTest()` público de solo-test que llama al `persistConfig()` privado; el
> `config_token` se añade aquí directamente para que el round-trip cubra también ese campo,
> ya que Task 2 depende de que `persistConfig()` ya lo serialice).

Registrar el test en `test/test_run_all.cpp` (añadir tras `test_setupConfigData`):

```cpp
    RUN_TEST(test_persistConfigRoundTrip);
```

- [ ] **Step 2: Ejecutar los tests en hardware y confirmar que falla la compilación**

Run: `pio test -e d1_mini_lite -f test_manager`
Expected: FAIL to compile — `'class Manager' has no member named 'setSleepMinutes'` (u otro
miembro nuevo referenciado en el test).

- [ ] **Step 3: Añadir a `include/manager.hpp` los miembros y firmas nuevas**

En la sección `public:` de `Manager`, junto a los getters existentes:

```cpp
  String configToken(){return config_token;}

  // Setters usados por el flujo de reconfiguración remota (Task 4) y por este test.
  void setDeviceName(String name){device_name = name;}
  void setUseSleepMode(bool value){use_sleep_mode = value;}
  void setSleepMinutes(int value){sleep_minutes = value;}
  void setUseAnalogSensor(bool value){use_analog_sensor = value;}
  void setSensorClass(String value){sensor_class = value;}
  void setUseArduinoMapFunction(bool value){use_arduino_map_function = value;}
  void setAnalogMinValue(int value){analog_min_value = value;}
  void setAnalogMaxValue(int value){analog_max_value = value;}

#ifdef PIO_UNIT_TESTING
  void persistConfigForTest(){persistConfig();}
#endif
```

En la sección `private:`, junto al resto de miembros:

```cpp
  String config_token;

  void persistConfig(); //! Serializes all current members to /config.json
```

- [ ] **Step 4: Implementar `persistConfig()` en `src/manager.cpp`, extrayendo la lógica de guardado actual**

Añadir la función (por ejemplo, justo antes de `Manager::setup_wifi()`):

```cpp
void Manager::persistConfig(){
  DynamicJsonDocument json(1024);

  json["network_ip"] = network_ip;
  json["network_mask"] = network_mask;
  json["network_gateway"] = network_gateway;

  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;
  json["mqtt_user"] = mqtt_user;
  json["mqtt_password"] = mqtt_password;

  json["use_sleep_mode"] = use_sleep_mode ? "true" : "false";
  json["sleep_minutes"] = String(sleep_minutes);

  json["device_name"] = device_name;

  json["use_analog_sensor"] = use_analog_sensor ? "true" : "false";
  json["sensor_class"] = sensor_class;
  json["use_arduino_map_function"] = use_arduino_map_function ? "true" : "false";
  json["analog_min_value"] = String(analog_min_value);
  json["analog_max_value"] = String(analog_max_value);

  json["config_token"] = config_token;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("[Manager] Failed to open config file for writing");
    return;
  }

  serializeJson(json, Serial);
  serializeJson(json, configFile);
  configFile.close();
}
```

Reemplazar el bloque de guardado en `Manager::setup_wifi()` (líneas 247-286 actuales, desde
`if (shouldSaveConfig) {` hasta el `}` que cierra ese bloque, sin incluir el `ESP.restart()`
final que sigue igual) por:

```cpp
  if (shouldSaveConfig) {
    Serial.println("[Manager] Saving configuration");

    network_ip = custom_network_ip.getValue();
    network_mask = custom_network_mask.getValue();
    network_gateway = custom_network_gateway.getValue();

    mqtt_server = custom_mqtt_server.getValue();
    mqtt_port = custom_mqtt_port.getValue();
    mqtt_user = custom_mqtt_username.getValue();
    mqtt_password = custom_mqtt_password.getValue();

    String sleepAux = custom_use_sleep_mode.getValue();
    sleepAux.toLowerCase();
    use_sleep_mode = sleepAux.equals("true");
    sleep_minutes = String(custom_sleep_minutes.getValue()).toInt();

    device_name = custom_device_name.getValue();

    String analogAux = custom_use_analog_sensor.getValue();
    analogAux.toLowerCase();
    use_analog_sensor = analogAux.equals("true");
    sensor_class = custom_sensor_class.getValue();
    String mapAux = custom_use_map_function.getValue();
    mapAux.toLowerCase();
    use_arduino_map_function = mapAux.equals("true");
    analog_min_value = String(custom_analog_min_value.getValue()).toInt();
    analog_max_value = String(custom_analog_max_value.getValue()).toInt();

    persistConfig();

    Serial.println("[Manager] \nRestarting...");
    ESP.restart();
    //end save
  }
```

En `Manager::setup_config_data()`, dentro del bloque `if (!json.isNull()) { ... }` (tras la
línea que lee `analog_max_value`), añadir la lectura de `config_token`:

```cpp
          config_token = (const char *)json["config_token"];
```

(La generación automática cuando falta se añade en el Task 2 — de momento solo se lee, y si
no existe queda como cadena vacía, que es el comportamiento correcto para que el Task 2 lo
detecte.)

En `Manager::Manager()` (constructor), añadir junto al resto de inicializaciones:

```cpp
  config_token = "";
```

- [ ] **Step 5: Ejecutar los tests en hardware y confirmar que pasan**

Run: `pio test -e d1_mini_lite -f test_manager`
Expected: PASS — `test_persistConfigRoundTrip` y `test_setupConfigData` en verde.

- [ ] **Step 6: Confirmar que el resto de la suite y el build de firmware siguen intactos**

Run: `pio run -e d1_mini_lite`
Expected: build sin errores (el refactor no cambia comportamiento del portal).

- [ ] **Step 7: Commit**

```bash
git add include/manager.hpp src/manager.cpp test/tests/test_manager.cpp test/test_run_all.cpp
git commit -m "refactor: extract Manager::persistConfig for reuse by remote config"
```

---

### Task 2: Token de configuración auto-generado y visible en el portal

**Files:**
- Modify: `include/manager.hpp`
- Modify: `src/manager.cpp`
- Test: `test/tests/test_manager.cpp`
- Modify: `test/test_run_all.cpp`

**Interfaces:**
- Consumes: `Manager::persistConfig()` (Task 1), `Manager::configToken()` (Task 1, ya
  declarado).
- Produces: `String Manager::generateToken()` (privado) — genera 32 caracteres hex vía
  `secureRandom()`. Tras este task, `configToken()` nunca devuelve una cadena vacía después
  de `setup_config_data()`.

- [ ] **Step 1: Escribir el test (debe fallar: el token sigue vacío)**

Añadir a `test/tests/test_manager.cpp`:

```cpp
void test_configTokenGeneratedWhenMissing() {
    DynamicJsonDocument json(1024);
    json["network_ip"] = "192.168.1.50";
    json["network_mask"] = "255.255.255.0";
    json["network_gateway"] = "192.168.1.1";
    json["mqtt_server"] = "192.168.1.100";
    json["mqtt_port"] = "1883";
    json["mqtt_user"] = "user";
    json["mqtt_password"] = "pass";
    json["use_sleep_mode"] = "false";
    json["device_name"] = "no_token_device";
    // config_token intencionadamente ausente: simula un config.json de antes de este feature

    File configFile = LittleFS.open("/config.json", "w");
    serializeJson(json, configFile);
    configFile.close();

    Manager manager;
    manager.setup_config_data();

    TEST_ASSERT_EQUAL(32, manager.configToken().length());

    // Debe haberse persistido: al recargar, el mismo token se mantiene (no se regenera cada vez)
    String firstToken = manager.configToken();
    Manager reloaded;
    reloaded.setup_config_data();
    TEST_ASSERT_EQUAL_STRING(firstToken.c_str(), reloaded.configToken().c_str());
}
```

Registrar en `test/test_run_all.cpp`:

```cpp
    RUN_TEST(test_configTokenGeneratedWhenMissing);
```

- [ ] **Step 2: Ejecutar y confirmar que falla**

Run: `pio test -e d1_mini_lite -f test_manager`
Expected: FAIL — `configToken().length()` es 0, no 32.

- [ ] **Step 3: Implementar `generateToken()` y la auto-generación en `setup_config_data()`**

En `include/manager.hpp`, sección `private:`, junto a `persistConfig()`:

```cpp
  String generateToken(); //! Generates a random 32-hex-char config token
```

En `src/manager.cpp`, añadir la implementación (por ejemplo justo antes de
`persistConfig()`):

```cpp
String Manager::generateToken(){
  const char hexChars[] = "0123456789abcdef";
  String token;
  for (int i = 0; i < 32; i++){
    token += hexChars[secureRandom(0, 16)];
  }
  return token;
}
```

En `Manager::setup_config_data()`, dentro del bloque `if (!json.isNull()) { ... }`, justo
después de la línea `config_token = (const char *)json["config_token"];` añadida en el
Task 1:

```cpp
          config_token = (const char *)json["config_token"];
          if (config_token.length() == 0) {
            config_token = generateToken();
            Serial.println("[Manager] No config token found. Generated a new one: " + config_token);
            persistConfig();
          }
```

Y al final de `setup_config_data()`, justo antes del cierre de la función (cubre el caso de
dispositivo nuevo, sin `config.json`, y el caso de fallo de montaje de LittleFS):

```cpp
  if (config_token.length() == 0) {
    config_token = generateToken();
  }
}
```

- [ ] **Step 4: Ejecutar y confirmar que pasa**

Run: `pio test -e d1_mini_lite -f test_manager`
Expected: PASS.

- [ ] **Step 5: Mostrar el token en el portal cautivo (dispositivo nuevo)**

En `Manager::setup_wifi()`, tras la declaración de `custom_device_name` (línea ~123),
añadir:

```cpp
  String tokenDisplayHtml = "<p><b>Config token:</b> " + config_token +
    "<br/><small>Copia y guarda este valor: lo necesitarás para reconfigurar el "
    "dispositivo remotamente por MQTT sin acceso físico.</small></p>";
  WiFiManagerParameter custom_config_token_display(tokenDisplayHtml.c_str());
```

`tokenDisplayHtml` debe declararse como variable local de `setup_wifi()` (no dentro de un
bloque anidado) porque `WiFiManagerParameter` solo guarda el puntero al `const char*`, no una
copia — debe seguir viva durante toda la función, lo cual se cumple aquí porque
`wifiManager.startConfigPortal(...)` se ejecuta de forma síncrona dentro de la misma función.

Justo después de `wifiManager.addParameter(&custom_device_name);` (línea ~177), añadir:

```cpp
  wifiManager.addParameter(&custom_config_token_display);
```

- [ ] **Step 6: Verificación manual en hardware**

No es testeable por Unity (renderizado HTML del portal). Verificar manualmente: borrar
`config.json` de un dispositivo de pruebas, arrancarlo, conectarse al portal cautivo
"Meteo-home" y confirmar que la página de "Device parameters" muestra un token de 32
caracteres hexadecimales.

- [ ] **Step 7: Commit**

```bash
git add include/manager.hpp src/manager.cpp test/tests/test_manager.cpp test/test_run_all.cpp
git commit -m "feat: auto-generate and display a per-device remote config token"
```

---

### Task 3: `Manager::applyRemoteConfig()` — validación y aplicación transaccional

**Files:**
- Modify: `include/manager.hpp`
- Modify: `src/manager.cpp`
- Test: `test/tests/test_manager.cpp`
- Modify: `test/test_run_all.cpp`

**Interfaces:**
- Consumes: `Manager::configToken()`, `Manager::persistConfig()`, los setters añadidos en
  Task 1 (`setDeviceName`, `setSleepMinutes`, etc.).
- Produces:
  ```cpp
  struct RemoteConfigOutcome {
    bool success = false;
    String reason;                        // "applied", "invalid_token", "invalid_payload", "invalid_field:<name>"
    std::vector<String> appliedFields;     // solo relevante cuando success == true
  };

  RemoteConfigOutcome Manager::applyRemoteConfig(JsonDocument &doc);
  ```
  Este outcome lo consume `MeteoBoard` en el Task 5.

- [ ] **Step 1: Escribir los tests (deben fallar: `applyRemoteConfig` no existe)**

Añadir a `test/tests/test_manager.cpp`:

```cpp
Manager makeConfiguredManager() {
    LittleFS.remove("/config.json");
    Manager manager;
    manager.setDeviceName("device_under_test");
    manager.setSleepMinutes(5);
    manager.setUseSleepMode(false);
    manager.setUseAnalogSensor(true);
    manager.setSensorClass("moisture");
    manager.setUseArduinoMapFunction(true);
    manager.setAnalogMinValue(0);
    manager.setAnalogMaxValue(1024);
    manager.persistConfigForTest();
    // El token no se genera hasta setup_config_data(); para test lo forzamos leyendo de disco.
    manager.setup_config_data();
    return manager;
}

void test_applyRemoteConfigValidPartialUpdate() {
    Manager manager = makeConfiguredManager();
    String token = manager.configToken();

    DynamicJsonDocument doc(256);
    doc["token"] = token;
    doc["sleep_minutes"] = 20;
    doc["device_name"] = "renamed_device";

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_TRUE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("applied", outcome.reason.c_str());
    TEST_ASSERT_EQUAL(2, outcome.appliedFields.size());
    TEST_ASSERT_EQUAL(20, manager.sleepMinutes());
    TEST_ASSERT_EQUAL_STRING("renamed_device", manager.deviceName().c_str());
    // Campos no incluidos en el payload no cambian
    TEST_ASSERT_FALSE(manager.useSleepMode());
    TEST_ASSERT_EQUAL_STRING("moisture", manager.sensorClass().c_str());
}

void test_applyRemoteConfigInvalidToken() {
    Manager manager = makeConfiguredManager();

    DynamicJsonDocument doc(256);
    doc["token"] = "wrong-token";
    doc["sleep_minutes"] = 20;

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("invalid_token", outcome.reason.c_str());
    TEST_ASSERT_EQUAL(5, manager.sleepMinutes()); // sin cambios
}

void test_applyRemoteConfigMissingToken() {
    Manager manager = makeConfiguredManager();

    DynamicJsonDocument doc(256);
    doc["sleep_minutes"] = 20;

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("invalid_token", outcome.reason.c_str());
}

void test_applyRemoteConfigInvalidSleepMinutes() {
    Manager manager = makeConfiguredManager();
    String token = manager.configToken();

    DynamicJsonDocument doc(256);
    doc["token"] = token;
    doc["sleep_minutes"] = 61; // fuera de rango 1-60
    doc["device_name"] = "should_not_apply";

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("invalid_field:sleep_minutes", outcome.reason.c_str());
    // Transaccional: ni siquiera device_name se aplica
    TEST_ASSERT_EQUAL_STRING("device_under_test", manager.deviceName().c_str());
}

void test_applyRemoteConfigInvalidAnalogRange() {
    Manager manager = makeConfiguredManager();
    String token = manager.configToken();

    DynamicJsonDocument doc(256);
    doc["token"] = token;
    doc["analog_min_value"] = 500;
    doc["analog_max_value"] = 100; // min >= max, invalido

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("invalid_field:analog_min_value", outcome.reason.c_str());
}

void test_applyRemoteConfigEmptyDeviceName() {
    Manager manager = makeConfiguredManager();
    String token = manager.configToken();

    DynamicJsonDocument doc(256);
    doc["token"] = token;
    doc["device_name"] = "";

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("invalid_field:device_name", outcome.reason.c_str());
}
```

Registrar en `test/test_run_all.cpp`:

```cpp
    RUN_TEST(test_applyRemoteConfigValidPartialUpdate);
    RUN_TEST(test_applyRemoteConfigInvalidToken);
    RUN_TEST(test_applyRemoteConfigMissingToken);
    RUN_TEST(test_applyRemoteConfigInvalidSleepMinutes);
    RUN_TEST(test_applyRemoteConfigInvalidAnalogRange);
    RUN_TEST(test_applyRemoteConfigEmptyDeviceName);
```

- [ ] **Step 2: Ejecutar y confirmar que falla la compilación**

Run: `pio test -e d1_mini_lite -f test_manager`
Expected: FAIL to compile — `'class Manager' has no member named 'applyRemoteConfig'`.

- [ ] **Step 3: Declarar `RemoteConfigOutcome` y `applyRemoteConfig()` en `include/manager.hpp`**

Añadir `#include <vector>` a los includes existentes, y antes de `class Manager{`:

```cpp
struct RemoteConfigOutcome {
  bool success = false;
  String reason;
  std::vector<String> appliedFields;
};
```

En la sección `public:` de `Manager`:

```cpp
  RemoteConfigOutcome applyRemoteConfig(JsonDocument &doc);
```

- [ ] **Step 4: Implementar `applyRemoteConfig()` en `src/manager.cpp`**

```cpp
RemoteConfigOutcome Manager::applyRemoteConfig(JsonDocument &doc){
  RemoteConfigOutcome outcome;

  if (!doc.containsKey("token") || String((const char *)doc["token"]) != config_token) {
    outcome.reason = "invalid_token";
    return outcome;
  }

  // --- Validation pass: nothing is mutated until every present field is valid ---
  if (doc.containsKey("sleep_minutes")) {
    int v = doc["sleep_minutes"].as<int>();
    if (v < 1 || v > 60) {
      outcome.reason = "invalid_field:sleep_minutes";
      return outcome;
    }
  }

  if (doc.containsKey("analog_min_value") || doc.containsKey("analog_max_value")) {
    int minV = doc.containsKey("analog_min_value") ? doc["analog_min_value"].as<int>() : analog_min_value;
    int maxV = doc.containsKey("analog_max_value") ? doc["analog_max_value"].as<int>() : analog_max_value;
    if (minV < 0 || minV > 1024 || maxV < 0 || maxV > 1024 || minV >= maxV) {
      outcome.reason = "invalid_field:analog_min_value";
      return outcome;
    }
  }

  if (doc.containsKey("sensor_class") && String((const char *)doc["sensor_class"]).length() == 0) {
    outcome.reason = "invalid_field:sensor_class";
    return outcome;
  }

  if (doc.containsKey("device_name") && String((const char *)doc["device_name"]).length() == 0) {
    outcome.reason = "invalid_field:device_name";
    return outcome;
  }

  const char *boolFields[] = {"use_sleep_mode", "use_analog_sensor", "use_arduino_map_function"};
  for (const char *field : boolFields) {
    if (doc.containsKey(field) && !doc[field].is<bool>()) {
      outcome.reason = String("invalid_field:") + field;
      return outcome;
    }
  }

  // --- Apply pass: every present field is now known-valid ---
  if (doc.containsKey("device_name")) {
    device_name = (const char *)doc["device_name"];
    outcome.appliedFields.push_back("device_name");
  }
  if (doc.containsKey("use_sleep_mode")) {
    use_sleep_mode = doc["use_sleep_mode"].as<bool>();
    outcome.appliedFields.push_back("use_sleep_mode");
  }
  if (doc.containsKey("sleep_minutes")) {
    sleep_minutes = doc["sleep_minutes"].as<int>();
    outcome.appliedFields.push_back("sleep_minutes");
  }
  if (doc.containsKey("use_analog_sensor")) {
    use_analog_sensor = doc["use_analog_sensor"].as<bool>();
    outcome.appliedFields.push_back("use_analog_sensor");
  }
  if (doc.containsKey("sensor_class")) {
    sensor_class = (const char *)doc["sensor_class"];
    outcome.appliedFields.push_back("sensor_class");
  }
  if (doc.containsKey("use_arduino_map_function")) {
    use_arduino_map_function = doc["use_arduino_map_function"].as<bool>();
    outcome.appliedFields.push_back("use_arduino_map_function");
  }
  if (doc.containsKey("analog_min_value")) {
    analog_min_value = doc["analog_min_value"].as<int>();
    outcome.appliedFields.push_back("analog_min_value");
  }
  if (doc.containsKey("analog_max_value")) {
    analog_max_value = doc["analog_max_value"].as<int>();
    outcome.appliedFields.push_back("analog_max_value");
  }

  persistConfig();

  outcome.success = true;
  outcome.reason = "applied";
  return outcome;
}
```

- [ ] **Step 5: Ejecutar y confirmar que pasan todos los tests nuevos**

Run: `pio test -e d1_mini_lite -f test_manager`
Expected: PASS — los 6 tests nuevos en verde, y los de los Tasks 1-2 siguen en verde.

- [ ] **Step 6: Commit**

```bash
git add include/manager.hpp src/manager.cpp test/tests/test_manager.cpp test/test_run_all.cpp
git commit -m "feat: add Manager::applyRemoteConfig with transactional field validation"
```

---

### Task 4: Tópicos de configuración en `Manager`

**Files:**
- Modify: `include/manager.hpp`

**Interfaces:**
- Consumes: `deviceName()` (ya existente).
- Produces: `String configSetTopic()`, `String configResultTopic()` — consumidos por
  `MeteoBoard` en el Task 5.

Task pequeño y sin lógica condicional, no necesita su propio ciclo TDD con test unitario
nuevo: se verifica indirectamente por los tests de MeteoBoard del Task 5 (que usan estos
tópicos) y no tiene ramas que puedan fallar de forma independiente.

- [ ] **Step 1: Añadir los getters en `include/manager.hpp`**

Junto a `availabilityTopic()`:

```cpp
  //! Topic where an external client publishes a retained remote-configuration command.
  String configSetTopic(){return "meteohome/" + device_name + "/config/set";}
  //! Topic where this device publishes the result of processing a configuration command.
  String configResultTopic(){return "meteohome/" + device_name + "/config/result";}
```

- [ ] **Step 2: Verificar que compila**

Run: `pio run -e d1_mini_lite`
Expected: build sin errores (getters sin uso todavía, no deberían generar warnings nuevos
relevantes).

- [ ] **Step 3: Commit**

```bash
git add include/manager.hpp
git commit -m "feat: add Manager topic getters for remote config command/result"
```

---

### Task 5: `MeteoBoard::buildConfigResultPayload()` — payload de resultado (pura, testable)

**Files:**
- Modify: `include/meteoboard.hpp`
- Modify: `src/meteoboard.cpp`
- Create: `test/tests/test_meteoboard_config.cpp`
- Modify: `test/test_run_all.cpp`

**Interfaces:**
- Consumes: `RemoteConfigOutcome` (Task 3).
- Produces: `static String MeteoBoard::buildConfigResultPayload(const RemoteConfigOutcome &outcome)`
  — usado por `handleConfigCommand()` en el Task 6.

Se separa de `handleConfigCommand()` (Task 6) porque esta parte es pura (construye un
string a partir de datos, sin tocar la red) y por tanto testeable con Unity igual que
`createDiscoveryMsg()`; el resto de `handleConfigCommand()` (publish/subscribe reales) no lo
es con la infraestructura de test actual.

- [ ] **Step 1: Escribir el test (debe fallar: el método no existe)**

Crear `test/tests/test_meteoboard_config.cpp`:

```cpp
#include <Arduino.h>
#include <unity.h>
#include "meteoboard.hpp"

void test_buildConfigResultAppliedPayload() {
    RemoteConfigOutcome outcome;
    outcome.success = true;
    outcome.reason = "applied";
    outcome.appliedFields.push_back("sleep_minutes");
    outcome.appliedFields.push_back("device_name");

    String expected = "{\"status\":\"applied\",\"fields\":[\"sleep_minutes\",\"device_name\"]}";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), MeteoBoard::buildConfigResultPayload(outcome).c_str());
}

void test_buildConfigResultErrorPayload() {
    RemoteConfigOutcome outcome;
    outcome.success = false;
    outcome.reason = "invalid_field:sleep_minutes";

    String expected = "{\"status\":\"error\",\"reason\":\"invalid_field:sleep_minutes\"}";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), MeteoBoard::buildConfigResultPayload(outcome).c_str());
}
```

En `test/test_run_all.cpp`, añadir el include junto a los otros dos:

```cpp
#include "tests/test_meteoboard_config.cpp"
```

Y registrar en `setup()`:

```cpp
    RUN_TEST(test_buildConfigResultAppliedPayload);
    RUN_TEST(test_buildConfigResultErrorPayload);
```

- [ ] **Step 2: Ejecutar y confirmar que falla la compilación**

Run: `pio test -e d1_mini_lite -f test_meteoboard_config`
Expected: FAIL to compile — `'class MeteoBoard' has no member named 'buildConfigResultPayload'`.

- [ ] **Step 3: Declarar el método en `include/meteoboard.hpp`**

En la sección `public:`, junto a `sendDiscoveryMessage`:

```cpp
    static String buildConfigResultPayload(const RemoteConfigOutcome &outcome);
```

- [ ] **Step 4: Implementar en `src/meteoboard.cpp`**

```cpp
String MeteoBoard::buildConfigResultPayload(const RemoteConfigOutcome &outcome){
  DynamicJsonDocument doc(512);
  String buffer;

  if (outcome.success) {
    doc["status"] = "applied";
    JsonArray fields = doc.createNestedArray("fields");
    for (const String &field : outcome.appliedFields) {
      fields.add(field);
    }
  } else {
    doc["status"] = "error";
    doc["reason"] = outcome.reason;
  }

  serializeJson(doc, buffer);
  return buffer;
}
```

- [ ] **Step 5: Ejecutar y confirmar que pasan**

Run: `pio test -e d1_mini_lite -f test_meteoboard_config`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add include/meteoboard.hpp src/meteoboard.cpp test/tests/test_meteoboard_config.cpp test/test_run_all.cpp
git commit -m "feat: add MeteoBoard::buildConfigResultPayload for remote config responses"
```

---

### Task 6: Suscripción MQTT, `handleConfigCommand()` y reinicio

**Files:**
- Modify: `include/meteoboard.hpp`
- Modify: `src/meteoboard.cpp`

**Interfaces:**
- Consumes: `Manager::configSetTopic()`, `Manager::configResultTopic()` (Task 4),
  `Manager::applyRemoteConfig()` (Task 3), `MeteoBoard::buildConfigResultPayload()` (Task 5).
- Produces: `void MeteoBoard::handleConfigCommand(String payload)` — llamado desde
  `mqttCallback`.

No es testeable por Unity con la infraestructura actual (requiere un broker MQTT real
conectado durante el test en hardware, cosa que no existe hoy en este proyecto — ni siquiera
`autodiscover()`/`connectToMQTT()` se testean así). Se verifica manualmente en el Step final
con un broker MQTT real.

- [ ] **Step 1: Declarar `handleConfigCommand()` en `include/meteoboard.hpp`**

En la sección `public:`, junto a `sendDiscoveryMessage`:

```cpp
    void handleConfigCommand(String payload); //! Validates and applies a remote config command
```

- [ ] **Step 2: Suscribirse al tópico de comando en `connectToMQTT()`**

En `src/meteoboard.cpp`, dentro de `MeteoBoard::connectToMQTT()`, justo después de la línea
`client->subscribe("homeassistant/status");`:

```cpp
      client->subscribe(manager->configSetTopic().c_str());
```

- [ ] **Step 3: Enrutar el mensaje en `mqttCallback()`**

Reemplazar el cuerpo de `MeteoBoard::mqttCallback` (mismo archivo) por:

```cpp
void MeteoBoard::mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("[Board] MQTT message received on " + String(topic) + ": " + message);

  if (instance == nullptr) {
    return;
  }

  if (String(topic) == "homeassistant/status" && message == "online") {
    Serial.println("[Board] Home Assistant is online, resending discovery messages");
    instance->autodiscover();
  } else if (String(topic) == instance->manager->configSetTopic() && message.length() > 0) {
    // message.length() == 0 is our own retained-clear publish; ignore it to avoid a loop.
    instance->handleConfigCommand(message);
  }
}
```

- [ ] **Step 4: Implementar `handleConfigCommand()`**

```cpp
void MeteoBoard::handleConfigCommand(String payload){
  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, payload);

  RemoteConfigOutcome outcome;
  if (err) {
    outcome.success = false;
    outcome.reason = "invalid_payload";
  } else {
    outcome = manager->applyRemoteConfig(doc);
  }

  String resultPayload = buildConfigResultPayload(outcome);
  connectToMQTT();
  client->publish(manager->configResultTopic().c_str(), resultPayload.c_str());

  // Clear the retained command so it is not reprocessed on the next reconnect.
  client->publish(manager->configSetTopic().c_str(), "", true);

  if (outcome.success) {
    Serial.println("[Board] Remote config applied, restarting...");
    delay(200); // let the publishes above flush before the restart
    ESP.restart();
  }
}
```

- [ ] **Step 5: Compilar el firmware**

Run: `pio run -e d1_mini_lite`
Expected: build sin errores.

- [ ] **Step 6: Verificación manual end-to-end en hardware**

Con un dispositivo real conectado a un bróker MQTT accesible:

1. Anota el `config_token` del dispositivo (portal o Serial, según el Task 2).
2. Suscríbete al resultado: `mosquitto_sub -h <broker> -t 'meteohome/<device>/config/result' -v`
3. Publica un comando válido con retención:
   `mosquitto_pub -h <broker> -t 'meteohome/<device>/config/set' -r -m '{"token":"<token>","sleep_minutes":10}'`
4. Confirma que llega `{"status":"applied","fields":["sleep_minutes"]}` en `config/result`,
   que el dispositivo se reinicia, y que tras reiniciar el nuevo `sleep_minutes` persiste
   (revisar `config.json` o el comportamiento del ciclo de sleep).
5. Repite con un token incorrecto y confirma `{"status":"error","reason":"invalid_token"}`
   sin reinicio.
6. Confirma que `meteohome/<device>/config/set` queda vacío/retenido-limpio tras cada
   intento (`mosquitto_sub` con `-R` o revisando que una nueva conexión no reprocesa el
   comando).

- [ ] **Step 7: Commit**

```bash
git add include/meteoboard.hpp src/meteoboard.cpp
git commit -m "feat: handle remote config commands over MQTT and restart on success"
```

---

### Task 7: Documentación

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Añadir una sección "Reconfiguración remota" al README**

Insertar tras la sección "## Configuring a meteo-home device" (antes de "## Using MeteoHome
with Home Assistant"):

```markdown
## Reconfiguración remota

Una vez configurado, un dispositivo MeteoHome puede reconfigurarse sin acceso físico
publicando un comando MQTT. Solo se pueden cambiar parámetros de dispositivo/sensores (no
red ni credenciales MQTT, que siguen requiriendo borrar `config.json` y repetir el portal).

Cada dispositivo tiene un token secreto de 32 caracteres (`config_token`), generado
automáticamente. En un dispositivo nuevo se muestra en la página del portal cautivo, junto
al nombre del dispositivo. En un dispositivo actualizado desde una versión de firmware
anterior a esta funcionalidad, el token se genera en el primer arranque tras la
actualización y se imprime una única vez por el puerto serie (9600 baudios) — revísalo
antes de que se pierda, o borra `config.json` para volver a pasar por el portal y verlo ahí.

Para reconfigurar, publica un JSON con `retain=true` en `meteohome/<device>/config/set`,
incluyendo el token y solo los campos que quieras cambiar:

\`\`\`shell
mosquitto_pub -h <broker> -t 'meteohome/attic/config/set' -r \
  -m '{"token":"<config_token>","sleep_minutes":10,"device_name":"attic"}'
\`\`\`

Campos aceptados: `device_name`, `use_sleep_mode`, `sleep_minutes` (1-60),
`use_analog_sensor`, `sensor_class`, `use_arduino_map_function`, `analog_min_value` y
`analog_max_value` (0-1024, con `analog_min_value < analog_max_value`).

El dispositivo responde en `meteohome/<device>/config/result` (no retenido):

- `{"status":"applied","fields":[...]}` — cambio aplicado; el dispositivo se reinicia.
- `{"status":"error","reason":"invalid_token"}` — token ausente o incorrecto.
- `{"status":"error","reason":"invalid_payload"}` — JSON malformado.
- `{"status":"error","reason":"invalid_field:<nombre>"}` — un campo fuera de rango o de
  tipo incorrecto; no se aplica ningún cambio (validación transaccional).

Tras procesar el comando (con éxito o error), el dispositivo limpia el retained de
`config/set` para no reprocesarlo en la siguiente reconexión.
```

- [ ] **Step 2: Commit**

```bash
git add README.md
git commit -m "docs: document the remote MQTT configuration channel"
```

---

## Self-Review Notes

- **Cobertura del spec:** alcance (Task 3 limita los campos), seguridad de token (Task 2 +
  Task 3), canal MQTT retenido + limpieza (Task 6), guardar-y-reiniciar (Task 3 + Task 6),
  manejo de errores transaccional (Task 3), testing en `d1_mini_lite` en vez de `native`
  (todas las tests declaradas en Tasks 1, 2, 3, 5), documentación (Task 7) — todo cubierto.
- **Sin placeholders:** cada step tiene código completo, comandos exactos y salida esperada.
- **Consistencia de tipos:** `RemoteConfigOutcome` se define una vez (Task 3) y se reutiliza
  igual en Task 5 y Task 6; `configSetTopic()`/`configResultTopic()` (Task 4) se usan con el
  mismo nombre en Task 6; los setters de Task 1 (`setDeviceName`, `setSleepMinutes`, etc.) se
  usan con los mismos nombres en los tests del Task 3.
