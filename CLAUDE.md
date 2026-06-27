# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

MeteoHome is firmware for an ESP8266 (Wemos D1 Mini Lite) weather station built with
PlatformIO + the Arduino framework. It reads environmental sensors, publishes readings to
an MQTT broker using Home Assistant autodiscovery conventions, and is configured at runtime
through a WiFiManager captive portal (no credentials are compiled in).

## Common commands

```shell
# Build firmware for the target board
pio run -e d1_mini_lite

# Flash over USB
pio run -e d1_mini_lite -t upload

# Flash over the network (OTA). Set upload_port (IP/mDNS) in platformio.ini first
pio run -e d1_mini_lite_ota -t upload

# Serial monitor (115200, with ESP8266 exception decoder)
pio device monitor

# Unit tests on hardware (uses real LittleFS on the device)
pio test -e d1_mini_lite

# Unit tests on host (native, ArduinoFake mock layer)
pio test -e native

# Static analysis
pio check -e d1_mini_lite --skip-packages --severity=medium

# Reproduce the CI build locally
pio ci -c platformio.ini -e d1_mini_lite -l include/ src/
```

Running a single test: all tests are registered as `RUN_TEST(...)` calls in
`test/test_run_all.cpp`. Comment out the others, or pass a test-name filter
(`pio test -e native -f "<name>"`), to narrow the run.

## Architecture

The dependency graph is wired in `src/main.cpp`, which acts as the composition root and is
the only translation unit that talks to hardware/WiFi directly. It is excluded from test
builds via the `PIO_UNIT_TESTING` guard (and `test_ignore` in `platformio.ini`).

Boot flow in `setup()`: load config from LittleFS → bring up WiFi/portal via `Manager` →
construct each sensor and call `begin()`; only sensors whose `begin()` succeeds are handed
to the board via `addSensor()` → `board.autodiscover()` → an OTA wait window → enter
`loop()`. The first-boot flag is stored in RTC user memory so discovery messages are only
sent on a cold boot, not after waking from deep sleep.

Core abstractions:

- **`Manager`** (`manager.{hpp,cpp}`) — owns runtime configuration (network, MQTT, device
  name, sleep mode). Loads/saves `/config.json` on LittleFS and drives the WiFiManager
  captive portal. The constructor sets fallback defaults; real values come from the portal.
- **`MeteoSensor`** (`meteosensor.hpp`) — abstract base for every sensor. Subclasses
  implement `available()`, `read()`, `getDiscoveryMsg()`, and `autodiscover()`.
  `createDiscoveryMsg()` builds the Home Assistant MQTT autodiscovery JSON payload shared by
  all sensors. Concrete sensors: `MHDHT` (DHT22), `MHBMP` (BMP085/180), `MHSGP30` (air
  quality), `MHAHT20`, and `MHVoltage` (MCU Vcc, via `ADC_MODE(ADC_VCC)`).
- **`MeteoBoard`** (`meteoboard.{hpp,cpp}`) — owns the `PubSubClient` and a
  `vector<unique_ptr<MeteoSensor>>`. `processSensors()` and `autodiscover()` iterate that
  vector. MQTT uses a retained Last-Will availability topic (`meteohome/status`). A static
  `instance` pointer lets the static `mqttCallback` reach the board: it subscribes to
  `homeassistant/status` and resends discovery when Home Assistant publishes its `online`
  birth message.
- **`Leds`** (`leds.{hpp,cpp}`) — drives the three status LEDs; `MHSGP30` uses them to show
  air-quality state.

Operating modes (chosen at runtime by the `use_sleep_mode` config value): with deep sleep,
`loop()` reads sensors once and calls `ESP.deepSleep(DEEP_SLEEP_TIME)`; without it, the loop
stays awake, servicing `client.loop()` and OTA between readings. `DEEP_SLEEP_TIME` is
defined in `manager.hpp`.

## Conventions & gotchas

- Sensors are registered conditionally: a sensor must have a working `begin()` to be added,
  so the active sensor set is hardware-dependent and discovered at boot.
- New sensors: subclass `MeteoSensor`, construct it with the board + manager (+ `Leds` if it
  needs the LEDs), then add a `begin()/addSensor()` block in `main.cpp` setup.
- `native` and `d1_mini_lite` test environments have different `lib_deps`; tests that pull in
  sensor libraries are intended for the on-hardware `d1_mini_lite` environment.
- MQTT topics are derived from the device/location name set in the portal (e.g.
  `attic/DHT22/temperature`); see `README.md` for the full topic layout and Home Assistant
  integration details.
- `configuration.yaml` is a sample Home Assistant config, not firmware.
