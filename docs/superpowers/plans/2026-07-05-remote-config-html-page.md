# Página HTML de reconfiguración remota Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Sustituir la plantilla de Home Assistant por una única página HTML autocontenida
(`tools/remote-config.html`) que lee el estado real de un dispositivo MeteoHome vía MQTT sobre
WebSockets y permite aplicar cambios de configuración sin componer JSON a mano.

**Architecture:** Un solo archivo HTML con CSS y JavaScript inline, con la librería MQTT.js
vendorizada dentro del propio archivo (sin `.js` externo ni CDN). Se conecta directamente al
broker desde el navegador; no hay backend ni dependencia de Home Assistant. El README pierde la
sección de plantilla de HA y gana instrucciones de uso de esta página.

**Tech Stack:** HTML5, CSS, JavaScript vanilla (sin framework), MQTT.js 5.15.1 (build de
navegador, vendorizada inline), Markdown (README).

## Global Constraints

- Archivo único: `tools/remote-config.html`. Nada de `.js` externo, nada de referencias a CDN.
- Librería vendorizada: `mqtt@5.15.1`, extraída de `package/dist/mqtt.min.js` dentro del
  tarball de `npm pack mqtt@5.15.1` (confirmado: expone la variable global `mqtt`, pasa
  `node --check`, ~369.000 bytes).
- Campos del formulario y del payload de `config/set`, en este orden: `device_name`,
  `use_sleep_mode`, `sleep_minutes`, `use_analog_sensor`, `sensor_class`,
  `use_arduino_map_function`, `analog_min_value`, `analog_max_value`. El payload de aplicar
  siempre incluye `token` + los 8 campos actuales del formulario (envío atómico, no parcial).
- Tópicos: `meteohome/<device>/config/state` (leer, suscripción), `meteohome/<device>/config/set`
  (escribir, publicación retenida), `meteohome/<device>/config/result` (leer resultado,
  suscripción) — mismos nombres ya usados por el firmware, sin cambios de protocolo.
- Claves de `localStorage`: `meteohome_remote_config_connection` (JSON con `host`, `port`,
  `wss`, `user`, `password`) y `meteohome_remote_config_token_<device_name>` (token en texto
  plano, uno por nombre de dispositivo usado).
- Sin test automatizado real: se verifica sintaxis JS con `node --check` sobre el contenido de
  los `<script>` extraído, y estructura HTML con `html.parser` de Python. La verificación
  interactiva end-to-end contra un broker y un dispositivo reales queda para el usuario
  (mismo criterio ya usado para el resto de piezas MQTT de este proyecto).
- El README pierde la sección `### Formulario en Home Assistant` (helpers/automatizaciones/
  script/tarjeta) y gana instrucciones de uso de `tools/remote-config.html`, incluyendo el
  requisito previo del listener WebSocket del broker. La documentación de
  `config/set`/`config/state`/`config/result` y el ejemplo de `mosquitto_pub` no cambian.

---

### Task 1: Esqueleto de la página, MQTT.js vendorizada y panel de conexión

**Files:**
- Create: `tools/remote-config.html`

**Interfaces:**
- Consumes: nada (primer archivo del feature).
- Produces: variable global `mqtt` (de la librería vendorizada, usada por `mqtt.connect(...)`
  en este y los siguientes tasks); elementos DOM con los `id` usados en este task
  (`broker-host`, `broker-port`, `broker-wss`, `broker-user`, `broker-password`, `connect-btn`,
  `connection-status`); función `connect()` y objeto `els` (Task 2 y 3 lo extienden).

- [ ] **Step 1: Escribir el esqueleto de la página**

  Crea `tools/remote-config.html` con este contenido exacto:

  ```html
  <!DOCTYPE html>
  <!--
  Copyright 2023 meteo-home

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
  -->
  <html lang="es">
  <head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>MeteoHome - Reconfiguración remota</title>
  <style>
    body { font-family: sans-serif; max-width: 600px; margin: 2rem auto; padding: 0 1rem; }
    section { border: 1px solid #ccc; border-radius: 8px; padding: 1rem; margin-bottom: 1.5rem; }
    label { display: block; margin-bottom: 0.75rem; }
    input[type="text"], input[type="password"], input[type="number"] { width: 100%; box-sizing: border-box; padding: 0.3rem; margin-top: 0.2rem; }
    button { padding: 0.5rem 1rem; margin-top: 0.5rem; cursor: pointer; }
    pre { white-space: pre-wrap; word-break: break-word; background: #f5f5f5; padding: 0.5rem; border-radius: 4px; }
  </style>
  </head>
  <body>
  <h1>MeteoHome &mdash; Reconfiguraci&oacute;n remota</h1>

  <section id="connection-section">
    <h2>1. Conexi&oacute;n al broker</h2>
    <label>Host
      <input id="broker-host" type="text" placeholder="192.168.1.100">
    </label>
    <label>Puerto (WebSocket)
      <input id="broker-port" type="number" value="9001">
    </label>
    <label>
      <input id="broker-wss" type="checkbox"> Usar WSS (TLS)
    </label>
    <label>Usuario MQTT
      <input id="broker-user" type="text">
    </label>
    <label>Contrase&ntilde;a MQTT
      <input id="broker-password" type="password">
    </label>
    <button id="connect-btn" type="button">Conectar</button>
    <p id="connection-status">Desconectado</p>
  </section>

  <script>
  // MQTT_JS_VENDORED_LIBRARY_PLACEHOLDER
  </script>
  <script>
  (function () {
    'use strict';

    var client = null;

    var STORAGE_CONNECTION_KEY = 'meteohome_remote_config_connection';

    var els = {
      host: document.getElementById('broker-host'),
      port: document.getElementById('broker-port'),
      wss: document.getElementById('broker-wss'),
      user: document.getElementById('broker-user'),
      password: document.getElementById('broker-password'),
      connectBtn: document.getElementById('connect-btn'),
      connectionStatus: document.getElementById('connection-status')
    };

    function setConnectionStatus(text) {
      els.connectionStatus.textContent = text;
    }

    function loadConnectionSettings() {
      var raw = localStorage.getItem(STORAGE_CONNECTION_KEY);
      if (!raw) return;
      var saved = JSON.parse(raw);
      els.host.value = saved.host || '';
      els.port.value = saved.port || 9001;
      els.wss.checked = !!saved.wss;
      els.user.value = saved.user || '';
      els.password.value = saved.password || '';
    }

    function saveConnectionSettings() {
      localStorage.setItem(STORAGE_CONNECTION_KEY, JSON.stringify({
        host: els.host.value,
        port: els.port.value,
        wss: els.wss.checked,
        user: els.user.value,
        password: els.password.value
      }));
    }

    function connect() {
      if (client) {
        client.end(true);
        client = null;
      }

      var protocol = els.wss.checked ? 'wss' : 'ws';
      var url = protocol + '://' + els.host.value + ':' + els.port.value;

      setConnectionStatus('Conectando...');

      client = mqtt.connect(url, {
        username: els.user.value || undefined,
        password: els.password.value || undefined,
        connectTimeout: 8000,
        reconnectPeriod: 0
      });

      client.on('connect', function () {
        setConnectionStatus('Conectado a ' + url);
        saveConnectionSettings();
      });

      client.on('error', function (err) {
        setConnectionStatus('Error de conexión: ' + err.message);
      });

      client.on('close', function () {
        setConnectionStatus('Desconectado');
      });
    }

    els.connectBtn.addEventListener('click', connect);

    loadConnectionSettings();
  })();
  </script>
  </body>
  </html>
  ```

- [ ] **Step 2: Vendorizar MQTT.js**

  Descarga la librería y extrae el build de navegador:

  ```bash
  mkdir -p /tmp/meteo-mqtt-vendor
  cd /tmp/meteo-mqtt-vendor
  npm pack mqtt@5.15.1 --silent
  tar xzf mqtt-5.15.1.tgz package/dist/mqtt.min.js
  cd - > /dev/null
  ls -la /tmp/meteo-mqtt-vendor/package/dist/mqtt.min.js
  ```

  Expected: un archivo de alrededor de 369.000 bytes. Si `npm pack mqtt@5.15.1` falla (p. ej.
  sin acceso a red), reporta BLOCKED indicando el error exacto — no sustituyas por una versión
  distinta sin confirmarlo.

  Inserta el contenido descargado en el marcador del HTML:

  ```bash
  node -e "
  const fs = require('fs');
  const htmlPath = 'tools/remote-config.html';
  const libPath = '/tmp/meteo-mqtt-vendor/package/dist/mqtt.min.js';
  let html = fs.readFileSync(htmlPath, 'utf8');
  const lib = fs.readFileSync(libPath, 'utf8');
  const marker = '// MQTT_JS_VENDORED_LIBRARY_PLACEHOLDER';
  if (!html.includes(marker)) { throw new Error('marker not found in ' + htmlPath); }
  html = html.replace(marker, lib);
  fs.writeFileSync(htmlPath, html);
  console.log('inlined', lib.length, 'bytes');
  "
  grep -c "MQTT_JS_VENDORED_LIBRARY_PLACEHOLDER" tools/remote-config.html
  ```

  Expected: el `node -e` imprime `inlined 369xxx bytes` y el `grep -c` devuelve `0` (marcador
  ya no existe).

- [ ] **Step 3: Verificar sintaxis JS y estructura HTML**

  ```bash
  TMPJS=$(mktemp /tmp/remote-config-extracted-XXXXXX.js)
  node -e "
  const fs = require('fs');
  const html = fs.readFileSync('tools/remote-config.html', 'utf8');
  const scripts = [...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].map(m => m[1]).join('\n;\n');
  fs.writeFileSync(process.argv[1], scripts);
  " "$TMPJS"
  node --check "$TMPJS"
  echo "JS_SYNTAX_OK"
  ```

  Expected: `JS_SYNTAX_OK` sin errores de `node --check`.

  ```bash
  python3 -c "
  import html.parser
  class Checker(html.parser.HTMLParser):
      pass
  with open('tools/remote-config.html', encoding='utf-8') as f:
      content = f.read()
  Checker().feed(content)
  print('HTML_PARSE_OK')
  "
  ```

  Expected: `HTML_PARSE_OK` sin excepciones.

  Si tienes acceso a un navegador gráfico, abre el archivo, introduce cualquier host/puerto
  (p. ej. `localhost` / `1`, un puerto que seguro no responde) y pulsa "Conectar": el texto de
  estado debe cambiar a "Conectando..." y después a un mensaje de error. Si no tienes navegador
  gráfico disponible en este entorno, indícalo en el informe — la verificación interactiva
  completa contra un broker real queda para el usuario.

- [ ] **Step 4: Commit**

  ```bash
  git add tools/remote-config.html
  git commit -m "feat: add remote-config HTML page skeleton with MQTT.js and connection panel"
  ```

---

### Task 2: Sección de dispositivo, lectura de config/state y formulario

**Files:**
- Modify: `tools/remote-config.html`

**Interfaces:**
- Consumes: objeto `els`, función `setConnectionStatus`, variable `client`, función `connect()`
  (Task 1).
- Produces: elementos DOM `device-name`, `device-token`, `read-state-btn`, `read-state-status`,
  `field-device_name`, `field-use_sleep_mode`, `field-sleep_minutes`, `field-use_analog_sensor`,
  `field-sensor_class`, `field-use_arduino_map_function`, `field-analog_min_value`,
  `field-analog_max_value`; funciones `setReadStateStatus(text)`, `loadTokenForDevice(deviceName)`,
  `populateForm(state)`, `subscribeToDevice(deviceName)`, `readState()`; variables `currentDevice`,
  `readStateTimer`; función `onMessage(topic, payloadBuffer)` (Task 3 la extiende con la rama de
  `config/result`).

- [ ] **Step 1: Insertar el HTML de dispositivo y formulario**

  En `tools/remote-config.html`, usa el Edit tool con:

  old_string:
  ```html
    <button id="connect-btn" type="button">Conectar</button>
    <p id="connection-status">Desconectado</p>
  </section>

  <script>
  ```

  new_string:
  ```html
    <button id="connect-btn" type="button">Conectar</button>
    <p id="connection-status">Desconectado</p>
  </section>

  <section id="device-section">
    <h2>2. Dispositivo</h2>
    <label>Nombre del dispositivo
      <input id="device-name" type="text">
    </label>
    <label>Token
      <input id="device-token" type="password">
    </label>
    <button id="read-state-btn" type="button">Leer estado actual</button>
    <p id="read-state-status"></p>
  </section>

  <section id="form-section">
    <h2>3. Configuraci&oacute;n</h2>
    <label>Nombre del dispositivo (device_name)
      <input id="field-device_name" type="text" maxlength="40">
    </label>
    <label>
      <input id="field-use_sleep_mode" type="checkbox"> Modo sleep (use_sleep_mode)
    </label>
    <label>Minutos de sleep, 1-60 (sleep_minutes)
      <input id="field-sleep_minutes" type="number" min="1" max="60">
    </label>
    <label>
      <input id="field-use_analog_sensor" type="checkbox"> Sensor anal&oacute;gico (use_analog_sensor)
    </label>
    <label>Clase de sensor (sensor_class)
      <input id="field-sensor_class" type="text" maxlength="20">
    </label>
    <label>
      <input id="field-use_arduino_map_function" type="checkbox"> Usar map() (use_arduino_map_function)
    </label>
    <label>Valor m&iacute;nimo anal&oacute;gico, 0-1024 (analog_min_value)
      <input id="field-analog_min_value" type="number" min="0" max="1024">
    </label>
    <label>Valor m&aacute;ximo anal&oacute;gico, 0-1024 (analog_max_value)
      <input id="field-analog_max_value" type="number" min="0" max="1024">
    </label>
  </section>

  <script>
  ```

- [ ] **Step 2: Añadir variables de estado del dispositivo**

  old_string:
  ```js
  (function () {
    'use strict';

    var client = null;

    var STORAGE_CONNECTION_KEY = 'meteohome_remote_config_connection';

    var els = {
  ```

  new_string:
  ```js
  (function () {
    'use strict';

    var client = null;
    var currentDevice = null;
    var readStateTimer = null;

    var STORAGE_CONNECTION_KEY = 'meteohome_remote_config_connection';
    var STORAGE_TOKEN_PREFIX = 'meteohome_remote_config_token_';
    var READ_STATE_TIMEOUT_MS = 5000;

    var els = {
  ```

- [ ] **Step 3: Extender el objeto `els` con los nuevos elementos**

  old_string:
  ```js
      connectBtn: document.getElementById('connect-btn'),
      connectionStatus: document.getElementById('connection-status')
    };
  ```

  new_string:
  ```js
      connectBtn: document.getElementById('connect-btn'),
      connectionStatus: document.getElementById('connection-status'),

      deviceName: document.getElementById('device-name'),
      deviceToken: document.getElementById('device-token'),
      readStateBtn: document.getElementById('read-state-btn'),
      readStateStatus: document.getElementById('read-state-status'),

      fieldDeviceName: document.getElementById('field-device_name'),
      fieldUseSleepMode: document.getElementById('field-use_sleep_mode'),
      fieldSleepMinutes: document.getElementById('field-sleep_minutes'),
      fieldUseAnalogSensor: document.getElementById('field-use_analog_sensor'),
      fieldSensorClass: document.getElementById('field-sensor_class'),
      fieldUseArduinoMapFunction: document.getElementById('field-use_arduino_map_function'),
      fieldAnalogMinValue: document.getElementById('field-analog_min_value'),
      fieldAnalogMaxValue: document.getElementById('field-analog_max_value')
    };
  ```

- [ ] **Step 4: Añadir `setReadStateStatus`**

  old_string:
  ```js
    function setConnectionStatus(text) {
      els.connectionStatus.textContent = text;
    }

    function loadConnectionSettings() {
  ```

  new_string:
  ```js
    function setConnectionStatus(text) {
      els.connectionStatus.textContent = text;
    }

    function setReadStateStatus(text) {
      els.readStateStatus.textContent = text;
    }

    function loadConnectionSettings() {
  ```

- [ ] **Step 5: Añadir `loadTokenForDevice`, `populateForm` y `onMessage`**

  old_string:
  ```js
    function saveConnectionSettings() {
      localStorage.setItem(STORAGE_CONNECTION_KEY, JSON.stringify({
        host: els.host.value,
        port: els.port.value,
        wss: els.wss.checked,
        user: els.user.value,
        password: els.password.value
      }));
    }

    function connect() {
  ```

  new_string:
  ```js
    function saveConnectionSettings() {
      localStorage.setItem(STORAGE_CONNECTION_KEY, JSON.stringify({
        host: els.host.value,
        port: els.port.value,
        wss: els.wss.checked,
        user: els.user.value,
        password: els.password.value
      }));
    }

    function loadTokenForDevice(deviceName) {
      var token = localStorage.getItem(STORAGE_TOKEN_PREFIX + deviceName);
      if (token) {
        els.deviceToken.value = token;
      }
    }

    function populateForm(state) {
      els.fieldDeviceName.value = state.device_name || '';
      els.fieldUseSleepMode.checked = !!state.use_sleep_mode;
      els.fieldSleepMinutes.value = state.sleep_minutes;
      els.fieldUseAnalogSensor.checked = !!state.use_analog_sensor;
      els.fieldSensorClass.value = state.sensor_class || '';
      els.fieldUseArduinoMapFunction.checked = !!state.use_arduino_map_function;
      els.fieldAnalogMinValue.value = state.analog_min_value;
      els.fieldAnalogMaxValue.value = state.analog_max_value;
    }

    function onMessage(topic, payloadBuffer) {
      if (!currentDevice) return;
      var payload = payloadBuffer.toString();

      if (topic === 'meteohome/' + currentDevice + '/config/state') {
        if (readStateTimer) {
          clearTimeout(readStateTimer);
          readStateTimer = null;
        }
        if (payload.length === 0) {
          setReadStateStatus('El dispositivo no tiene estado retenido todavía.');
          return;
        }
        populateForm(JSON.parse(payload));
        setReadStateStatus('Estado leído correctamente.');
      }
    }

    function connect() {
  ```

- [ ] **Step 6: Suscribir a los mensajes MQTT y añadir `subscribeToDevice`/`readState`**

  old_string:
  ```js
      client.on('close', function () {
        setConnectionStatus('Desconectado');
      });
    }

    els.connectBtn.addEventListener('click', connect);

    loadConnectionSettings();
  })();
  ```

  new_string:
  ```js
      client.on('close', function () {
        setConnectionStatus('Desconectado');
      });

      client.on('message', onMessage);
    }

    function subscribeToDevice(deviceName) {
      if (currentDevice && currentDevice !== deviceName) {
        client.unsubscribe('meteohome/' + currentDevice + '/config/state');
        client.unsubscribe('meteohome/' + currentDevice + '/config/result');
      }
      currentDevice = deviceName;
      client.subscribe('meteohome/' + deviceName + '/config/state');
      client.subscribe('meteohome/' + deviceName + '/config/result');
    }

    function readState() {
      if (!client || !client.connected) {
        setReadStateStatus('Conecta primero al broker.');
        return;
      }
      var deviceName = els.deviceName.value.trim();
      if (!deviceName) {
        setReadStateStatus('Introduce el nombre del dispositivo.');
        return;
      }

      loadTokenForDevice(deviceName);
      subscribeToDevice(deviceName);

      setReadStateStatus('Esperando estado del dispositivo...');

      if (readStateTimer) clearTimeout(readStateTimer);
      readStateTimer = setTimeout(function () {
        setReadStateStatus('No se ha recibido estado del dispositivo. Comprueba el nombre y que esté conectado a MQTT.');
      }, READ_STATE_TIMEOUT_MS);
    }

    els.connectBtn.addEventListener('click', connect);
    els.readStateBtn.addEventListener('click', readState);
    els.deviceName.addEventListener('change', function () {
      loadTokenForDevice(els.deviceName.value.trim());
    });

    loadConnectionSettings();
  })();
  ```

- [ ] **Step 7: Verificar sintaxis JS y estructura HTML**

  Repite exactamente los dos comandos de verificación del Task 1 Step 3 (extracción +
  `node --check`, y el chequeo con `html.parser`). Expected: `JS_SYNTAX_OK` y `HTML_PARSE_OK`
  en ambos casos, sin errores.

- [ ] **Step 8: Commit**

  ```bash
  git add tools/remote-config.html
  git commit -m "feat: add device section, config/state read-back and config form"
  ```

---

### Task 3: Aplicar cambios, resultado y persistencia del token por dispositivo

**Files:**
- Modify: `tools/remote-config.html`

**Interfaces:**
- Consumes: objeto `els`, funciones `setReadStateStatus`, `subscribeToDevice`,
  `loadTokenForDevice`, `onMessage`, variable `currentDevice` (Task 2).
- Produces: elementos DOM `apply-btn`, `result-output`; funciones `saveTokenForDevice(deviceName,
  token)`, `showResult(result)`, `applyConfig()`. Ningún task posterior consume estos símbolos
  (última pieza del feature).

- [ ] **Step 1: Insertar el botón "Aplicar" y la sección de resultado**

  old_string:
  ```html
    <label>Valor m&aacute;ximo anal&oacute;gico, 0-1024 (analog_max_value)
      <input id="field-analog_max_value" type="number" min="0" max="1024">
    </label>
  </section>

  <script>
  ```

  new_string:
  ```html
    <label>Valor m&aacute;ximo anal&oacute;gico, 0-1024 (analog_max_value)
      <input id="field-analog_max_value" type="number" min="0" max="1024">
    </label>
    <button id="apply-btn" type="button">Aplicar cambios</button>
  </section>

  <section id="result-section">
    <h2>Resultado</h2>
    <pre id="result-output">(sin resultados todav&iacute;a)</pre>
  </section>

  <script>
  ```

- [ ] **Step 2: Extender el objeto `els`**

  old_string:
  ```js
      fieldAnalogMinValue: document.getElementById('field-analog_min_value'),
      fieldAnalogMaxValue: document.getElementById('field-analog_max_value')
    };
  ```

  new_string:
  ```js
      fieldAnalogMinValue: document.getElementById('field-analog_min_value'),
      fieldAnalogMaxValue: document.getElementById('field-analog_max_value'),
      applyBtn: document.getElementById('apply-btn'),

      resultOutput: document.getElementById('result-output')
    };
  ```

- [ ] **Step 3: Añadir `saveTokenForDevice`**

  old_string:
  ```js
    function loadTokenForDevice(deviceName) {
      var token = localStorage.getItem(STORAGE_TOKEN_PREFIX + deviceName);
      if (token) {
        els.deviceToken.value = token;
      }
    }
  ```

  new_string:
  ```js
    function loadTokenForDevice(deviceName) {
      var token = localStorage.getItem(STORAGE_TOKEN_PREFIX + deviceName);
      if (token) {
        els.deviceToken.value = token;
      }
    }

    function saveTokenForDevice(deviceName, token) {
      if (!deviceName || !token) return;
      localStorage.setItem(STORAGE_TOKEN_PREFIX + deviceName, token);
    }
  ```

- [ ] **Step 4: Extender `onMessage` con la rama de `config/result` y añadir `showResult`**

  old_string:
  ```js
        populateForm(JSON.parse(payload));
        setReadStateStatus('Estado leído correctamente.');
      }
    }

    function connect() {
  ```

  new_string:
  ```js
        populateForm(JSON.parse(payload));
        setReadStateStatus('Estado leído correctamente.');
      } else if (topic === 'meteohome/' + currentDevice + '/config/result') {
        if (payload.length === 0) return;
        showResult(JSON.parse(payload));
      }
    }

    function showResult(result) {
      els.resultOutput.textContent = JSON.stringify(result, null, 2);
    }

    function connect() {
  ```

- [ ] **Step 5: Añadir `applyConfig`**

  old_string:
  ```js
      if (readStateTimer) clearTimeout(readStateTimer);
      readStateTimer = setTimeout(function () {
        setReadStateStatus('No se ha recibido estado del dispositivo. Comprueba el nombre y que esté conectado a MQTT.');
      }, READ_STATE_TIMEOUT_MS);
    }

    els.connectBtn.addEventListener('click', connect);
    els.readStateBtn.addEventListener('click', readState);
    els.deviceName.addEventListener('change', function () {
      loadTokenForDevice(els.deviceName.value.trim());
    });

    loadConnectionSettings();
  })();
  ```

  new_string:
  ```js
      if (readStateTimer) clearTimeout(readStateTimer);
      readStateTimer = setTimeout(function () {
        setReadStateStatus('No se ha recibido estado del dispositivo. Comprueba el nombre y que esté conectado a MQTT.');
      }, READ_STATE_TIMEOUT_MS);
    }

    function applyConfig() {
      if (!client || !client.connected) {
        setReadStateStatus('Conecta primero al broker.');
        return;
      }
      var deviceName = els.deviceName.value.trim();
      if (!deviceName) {
        setReadStateStatus('Introduce el nombre del dispositivo.');
        return;
      }
      var token = els.deviceToken.value.trim();
      if (!token) {
        setReadStateStatus('Introduce el token del dispositivo.');
        return;
      }

      if (currentDevice !== deviceName) {
        subscribeToDevice(deviceName);
      }

      saveTokenForDevice(deviceName, token);

      var payload = {
        token: token,
        device_name: els.fieldDeviceName.value,
        use_sleep_mode: els.fieldUseSleepMode.checked,
        sleep_minutes: parseInt(els.fieldSleepMinutes.value, 10),
        use_analog_sensor: els.fieldUseAnalogSensor.checked,
        sensor_class: els.fieldSensorClass.value,
        use_arduino_map_function: els.fieldUseArduinoMapFunction.checked,
        analog_min_value: parseInt(els.fieldAnalogMinValue.value, 10),
        analog_max_value: parseInt(els.fieldAnalogMaxValue.value, 10)
      };

      client.publish('meteohome/' + deviceName + '/config/set', JSON.stringify(payload), { retain: true });
      els.resultOutput.textContent = 'Comando enviado. Si el dispositivo está en deep-sleep, se procesará en su próximo ciclo de conexión.';
    }

    els.connectBtn.addEventListener('click', connect);
    els.readStateBtn.addEventListener('click', readState);
    els.applyBtn.addEventListener('click', applyConfig);
    els.deviceName.addEventListener('change', function () {
      loadTokenForDevice(els.deviceName.value.trim());
    });

    loadConnectionSettings();
  })();
  ```

- [ ] **Step 6: Verificar sintaxis JS y estructura HTML**

  Repite exactamente los dos comandos de verificación del Task 1 Step 3. Expected:
  `JS_SYNTAX_OK` y `HTML_PARSE_OK`, sin errores.

- [ ] **Step 7: Commit**

  ```bash
  git add tools/remote-config.html
  git commit -m "feat: add apply/result flow and per-device token persistence"
  ```

---

### Task 4: Documentación — sustituir la plantilla de Home Assistant en el README

**Files:**
- Modify: `README.md`

**Interfaces:**
- Consumes: `tools/remote-config.html` (Tasks 1-3), tópicos ya documentados
  `config/set`/`config/state`/`config/result`.
- Produces: ninguno (tarea de documentación).

- [ ] **Step 1: Confirmar los límites exactos de la sección a sustituir**

  ```bash
  grep -n "^### Formulario en Home Assistant$" README.md
  grep -n "^## Using MeteoHome with Home Assistant$" README.md
  ```

  Anota los dos números de línea (a fecha de escribir este plan son 183 y 341; si difieren,
  usa los valores reales que devuelva el `grep`). La sección a sustituir va desde la primera
  línea (inclusive) hasta la línea inmediatamente anterior a la segunda coincidencia
  (inclusive; será una línea en blanco).

- [ ] **Step 2: Sustituir la sección**

  Usa el número de línea inicial (`N1`, la línea de `### Formulario en Home Assistant`) y el
  número de línea final (`N2`, la línea en blanco justo antes de
  `## Using MeteoHome with Home Assistant`) obtenidos en el Step 1. Borra ese rango:

  ```bash
  sed -i "${N1},${N2}d" README.md
  ```

  Sustituye `${N1}` y `${N2}` por los números reales antes de ejecutar (p. ej.
  `sed -i '183,340d' README.md` si el Step 1 confirmó esos valores).

  Verifica que el borrado dejó el archivo en el punto correcto:

  ```bash
  sed -n "$((N1-3)),$((N1+1))p" README.md
  ```

  Expected: las últimas líneas del párrafo anterior ("Este tópico permite construir un
  formulario...") seguidas directamente de `## Using MeteoHome with Home Assistant`.

  Ahora inserta el contenido nuevo con el Edit tool, anclando en el mismo punto:

  old_string:
  ```markdown
  Este tópico permite construir un formulario (por ejemplo, en Home Assistant) que se
  autorrellena con los valores reales del dispositivo antes de enviar un cambio.

  ## Using MeteoHome with Home Assistant
  ```

  new_string:
  ```markdown
  Este tópico permite construir un formulario (por ejemplo, en Home Assistant) que se
  autorrellena con los valores reales del dispositivo antes de enviar un cambio.

  ### Página de reconfiguración remota

  En `tools/remote-config.html` hay una página autocontenida (HTML + JS, sin backend ni
  dependencias externas) que lee el estado de un dispositivo y aplica cambios sin componer
  JSON a mano. Se abre directamente como archivo local en el navegador (doble clic o
  `file://`).

  **Requisito previo (una sola vez, en tu broker):** habilitar un listener WebSocket. En
  Mosquitto, añade a la configuración:

  ```
  listener 9001
  protocol websockets
  ```

  y reinicia el servicio. Sin esto, la página no podrá conectar.

  **Uso:**
  1. Abre `tools/remote-config.html` en el navegador.
  2. En "Conexión al broker", introduce el host y el puerto WebSocket de tu broker (el `9001`
     del ejemplo anterior), y el usuario/contraseña MQTT si tu broker los requiere. Pulsa
     "Conectar".
  3. En "Dispositivo", introduce el `device_name` del dispositivo y su token, y pulsa "Leer
     estado actual" — el formulario se rellenará con los valores reales del dispositivo.
  4. Edita los campos que quieras cambiar y pulsa "Aplicar cambios". El dispositivo aplicará
     el cambio (o devolverá un error) y el resultado aparecerá en la sección "Resultado".

  Los datos de conexión y el token de cada dispositivo se guardan en el `localStorage` del
  navegador donde abras la página, para no tener que reintroducirlos cada vez — igual nivel de
  confianza que ya asumías guardando esas mismas credenciales en otro sitio (p. ej. Home
  Assistant). Si expones el listener WebSocket del broker más allá de tu red local, hazlo bajo
  tu propio criterio de seguridad; esta página no añade ninguna protección adicional sobre el
  token ya existente.

  ## Using MeteoHome with Home Assistant
  ```

- [ ] **Step 3: Revisión visual**

  ```bash
  grep -n "^##\|^###" README.md
  ```

  Expected: ya no aparece `### Formulario en Home Assistant`; sí aparece
  `### Página de reconfiguración remota` entre `## Reconfiguración remota` y
  `## Using MeteoHome with Home Assistant`.

- [ ] **Step 4: Commit**

  ```bash
  git add README.md
  git commit -m "docs: replace Home Assistant template with remote-config HTML page instructions"
  ```

- [ ] **Step 5: Checklist de verificación manual para el usuario (incluir en el informe final)**

  Esta página no tiene test automatizado real (ver Global Constraints). Incluye literalmente
  este checklist en el informe final para que el controlador se lo traslade al usuario:

  - [ ] Habilitar el listener WebSocket en el broker (ver README).
  - [ ] Abrir `tools/remote-config.html`, conectar con host/puerto/credenciales reales.
  - [ ] Conectar con credenciales incorrectas y confirmar que se muestra un error.
  - [ ] Con un dispositivo real (o un `mosquitto_pub -r` simulando `config/state`), pulsar
        "Leer estado actual" y confirmar que el formulario se rellena.
  - [ ] Pulsar "Leer estado actual" con un nombre de dispositivo inexistente y confirmar el
        mensaje de timeout tras ~5s.
  - [ ] Aplicar un cambio válido contra un dispositivo real y confirmar que llega el resultado
        `applied` con los campos esperados.
  - [ ] Aplicar un cambio con token incorrecto y confirmar que llega `invalid_token`.
  - [ ] Recargar la página y confirmar que los datos de conexión y el token del dispositivo
        siguen rellenos (persistencia en `localStorage`).
