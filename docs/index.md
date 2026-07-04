---
layout: default
---

![workflow status](https://github.com/deuteros76/meteo-home/actions/workflows/main.yml/badge.svg)

# meteo-home
MeteoHome was born as a weather station project using a NodeMCU ESP8266 devboard with DHT22 and BMPP180 sensors. Now, the concept has changed and the board (currently a Wemos D1 Lite) and the source are being redesigned to allow the use of different type of sensors. One of the main features of MeteoHome is the configuration using a captive web portal and the use of MQTT to send the information to a server. Additionally, it has been designed to be connected to a machine running Home Assistant. In this case, this software could be used to show the current status of the sensors and display graphs.

## Features
- configuration of WiFi network and MQTT through a web portal
- Deep sleep mode included for being powered by battery
- Temperature, humidity, air pressure and air quality measurements depending on the sensors plugged to the board ( currently DHT22, BMP180 or SGP30)
- Possibility of being used with Home Assistant

## Future
The idea is to continue the development of both hardware and software. Currently the MCU is a Wemos D1 Lite but the plan is to extend the possibilites including an ESP32 or a Raspberry Pi Pico, At the same time, the sofware will accept more sensors and will be adapted to the new hardware.

- Integration of light sensor
- Integration of ATH20 sensor
- Adoption of a new MCU (ESP32 or Raspberry Pi Pico)
- Hardware redesign for the new MCU
- Development of software for the new board

## Requirements
- PlatformIO IDE (used to build, test and check quality of code)
- Additional libraries: WiFiManager, Arduino Json, PubSubClient, DHT and Adafruit BMP085 (see platformio.ini)
- A computer running MQTT
- [Optional] A computer running Home Assistant

