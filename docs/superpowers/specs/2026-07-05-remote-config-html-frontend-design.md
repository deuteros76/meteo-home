# Diseño: página HTML autocontenida para reconfiguración remota

**Fecha:** 2026-07-05
**Estado:** Aprobado para planificación
**Sustituye a:** la sección "Home Assistant" de
[`2026-07-05-remote-config-frontend-design.md`](2026-07-05-remote-config-frontend-design.md)
(helpers/automatizaciones/script/tarjeta). El tópico `meteohome/<device>/config/state` y su
publicación en firmware, ya implementados según ese documento, **no cambian** y son la base de
este diseño.

## Problema

La plantilla de Home Assistant (9 helpers, una automatización de sincronización, un script y
una tarjeta Lovelace, repetido por cada dispositivo) resultó ser demasiada configuración manual
por dispositivo. Se busca una alternativa con mucho menos trabajo de configuración por
dispositivo, sin perder la posibilidad de leer el estado real antes de cambiarlo ni el envío
atómico de todos los campos a la vez.

## Alcance

Una única página HTML autocontenida, sin backend ni dependencia de Home Assistant, que hable
MQTT directamente desde el navegador vía WebSockets. Sustituye la plantilla de HA como único
método documentado en el README para la reconfiguración remota práctica (`mosquitto_pub` a mano
sigue siendo válido y queda documentado como hasta ahora, para quien lo prefiera).

Explícitamente **fuera de alcance**:
- Cualquier backend, servidor local o infraestructura de hosting: la página se abre como
  archivo local (`file://`).
- Cambios al protocolo `config/set`/`config/state`/`config/result` ya existente: se reutiliza
  tal cual.
- Configurar el listener WebSocket del broker: es un requisito previo de infraestructura del
  usuario, documentado pero no automatizable desde este repositorio.

## Requisito previo (infraestructura, fuera del repo)

El broker MQTT necesita un listener WebSocket habilitado (p. ej. en Mosquitto, añadir a su
configuración `listener 9001` y `protocol websockets`). Sin esto, la página no puede conectar.
Se documenta como paso previo en el README; no es algo que el firmware o la página puedan
resolver.

## Arquitectura

Archivo único: `tools/remote-config.html`. Contiene HTML, CSS y JavaScript, con la librería
MQTT.js (build de navegador, UMD) vendorizada **inline** dentro de una etiqueta `<script>` del
propio archivo — no hay un `.js` separado ni referencia a ningún CDN. Esto evita dos problemas:
rutas relativas que se rompen si el archivo se copia/mueve (a un móvil, un USB...), y la
dependencia de acceso a internet más allá de la red local donde está el broker.

La página no tiene build ni dependencias de instalación: se abre directamente en el navegador
como archivo local.

## Componentes de la página

- **Conexión**: host y puerto del broker, checkbox WS/WSS (TLS), usuario y contraseña MQTT.
  Botón "Conectar".
- **Dispositivo**: nombre del dispositivo y token (campo tipo contraseña). Botón "Leer estado
  actual".
- **Formulario**: los 8 campos reconfigurables — `device_name`, `use_sleep_mode`,
  `sleep_minutes`, `use_analog_sensor`, `sensor_class`, `use_arduino_map_function`,
  `analog_min_value`, `analog_max_value`.
- **Botón "Aplicar cambios"**: construye el JSON completo (token + los 8 campos del formulario
  tal cual están en ese momento) y lo publica retenido en `config/set`. Envío atómico de todos
  los campos a la vez, igual que el ejemplo de `mosquitto_pub` ya documentado — no hay
  casillas de "incluir campo".
- **Resultado**: panel que muestra la respuesta de `config/result` en cuanto llega.
- **Persistencia (`localStorage`)**: datos de conexión al broker, y por cada nombre de
  dispositivo usado, su token — para no tener que reintroducirlos en visitas posteriores.

## Flujo de datos

```
Abrir la página (archivo local) → datos de conexión → "Conectar"
                                    │
                                    ▼
Nombre de dispositivo (autorrelleno de token si ya se usó antes) → "Leer estado actual"
                                    │
                                    ▼
Suscripción a config/state (retenido) → formulario se rellena con el estado real
                                    │
                                    ▼
Editar campos → "Aplicar cambios" → publicar JSON completo retenido en config/set
                                    │
                                    ▼
Suscripción a config/result → se muestra el resultado en cuanto el dispositivo responde
```

## Manejo de errores

| Situación | Comportamiento |
|---|---|
| Fallo de conexión WebSocket (broker inalcanzable o sin listener WS) | Mensaje visible sugiriendo comprobar el listener WS del broker |
| Fallo de autenticación MQTT | Mensaje de error al conectar (evento `error` de MQTT.js) |
| Sin respuesta a "Leer estado actual" tras ~5s | "No se ha recibido estado del dispositivo. Comprueba el nombre y que esté conectado a MQTT." |
| `config/result` con `status: "error"` | Se muestra literalmente el `reason` devuelto por el firmware (`invalid_token`, `invalid_payload`, `invalid_field:<campo>`), sin reinterpretar |
| Sin respuesta a `config/set` (p. ej. dispositivo en deep-sleep) | No es un error: nota informativa de que el comando queda retenido y se procesará en el próximo ciclo de conexión del dispositivo |

## Seguridad

- El token y las credenciales MQTT quedan en `localStorage` del navegador donde se abra la
  página, en texto plano — mismo nivel de confianza que ya se asumía al guardar esas mismas
  credenciales en Home Assistant; no es una relajación de seguridad, solo cambia dónde vive el
  mismo dato bajo el control del usuario.
- La página no introduce superficie de ataque nueva: solo habla MQTT con el broker indicado,
  igual que ya hacía `mosquitto_pub` manualmente. El token del dispositivo sigue siendo el único
  mecanismo de autorización de `config/set`, sin cambios respecto al diseño original.
- El listener WebSocket del broker es un puerto adicional de acceso a MQTT. Exponerlo más allá
  de la red local (p. ej. mediante port-forwarding) es una decisión de infraestructura del
  usuario, ajena a esta página; se documentará como aviso en el README.

## Testing

Página estática sin build ni framework: no hay tests automatizados. Verificación manual
(documentada como checklist en el plan de implementación):
- Conexión correcta e incorrecta (credenciales/host erróneos).
- Lectura de `config/state` de un dispositivo real (o simulado con `mosquitto_pub -r` publicando
  un payload de estado a mano, si no hay hardware disponible en el momento de implementar).
- Aplicación de un cambio y recepción del resultado, tanto en caso de éxito como de error
  (token incorrecto, campo fuera de rango).
- Persistencia en `localStorage` tras recargar la página.

Mismo criterio de verificación manual ya usado para el resto de piezas dependientes de MQTT en
este proyecto.

## Documentación

Se actualiza `README.md`: se elimina la sección de plantilla de Home Assistant (helpers,
automatizaciones, script, tarjeta) añadida en la iteración anterior, y se sustituye por
instrucciones de uso de `tools/remote-config.html`, incluyendo el requisito previo del listener
WebSocket del broker. La documentación de `config/set`/`config/state`/`config/result` y
`mosquitto_pub` como alternativa manual se mantiene sin cambios.
