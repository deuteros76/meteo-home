---
layout: page
title: Using MeteoHome with Home Assistant
---
A MeteoHome device publishes the data it collects to a MQTT broker. The topics used follow the specifications made by Home Assistant project and it is possible to use this software to store and use MeteoHome produced data. When a MeteoHome board is connected to a power source, it sends Home Assisant autodiscovery messages making it available through the MQTT integration.


![Home Assistant MQTT integration](pics/meteo-home-mqtt-integration.png) 

If Home Assistant is not your prefered option, any other software that includes a MQTT broker could be used to obtain the data. To get the data, the software should be configured to subscribe to the appropriate topics. As an example the following topics are availabe if a DHT22 and SGP-30 sensors are plugged to a board configred with "attic" as topic prefix:


| Sensor  | Topic  |   |   |   |
|---|---|---|---|---|
| DHT22  | attic/DHT22/temperature  | DHT22 tenperature value  |   |   |
| DHT22  | attic/DHT22/humidity  | DHT22 humidity value  |   |   |
| DHT22  | attic/DHT22/heatindex  | DHT22 heat index value  |   |   |
| SGP30  | attic/SGP30/co2  | SGP30 CO2 ppp  |   |   |
| SGP30  | attic/SGP30/voc  | SGP30 air quality |   |   |
| ESP8266  | attic/ESP/vcc  | ESP8266/microcontroller voltage received  |   |   |
