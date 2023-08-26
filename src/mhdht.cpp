/*
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
*/ 

#include "mhdht.hpp"

MHDHT::MHDHT(uint8_t pin, uint8_t type): DHT(pin, type){
  temperature_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) +"/DHT22-temperature/config";
  humidity_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/DHT22-humidity/config";
  heatindex_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/DHT22-heatindex/config";

  read(); // this reading is to make available() work from the begining
}

bool MHDHT::available(){
  bool returnValue=true;

  if (isnan(temperature) || isnan(humidity)){
    returnValue=false;
  }
  return returnValue;
;
}

void MHDHT::read(){
    //read dht22 value
  temperature = readTemperature();    
  humidity = readHumidity();
  heatindex = computeHeatIndex(temperature, humidity, false);
     
  Serial.print(temperature);
  Serial.print(" ");
  Serial.print(humidity);
  Serial.print(" ");
  Serial.println(heatindex);
}

String MHDHT::getDiscoveryMsg(String deviceName, deviceClass dev_class){
  String topic, unit, className;

  switch (dev_class){
    case temperature_sensor: unit = "ºC"; className="temperature"; topic= deviceName+"/DHT22/temperature"; break;
    case humidity_sensor: unit = "%"; className="humidity"; topic= deviceName+"/DHT22/humidity"; break;
    default: break;
  }

  return createDiscoveryMsg(topic, className, unit);
}
