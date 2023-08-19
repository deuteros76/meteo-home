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

#include "mhsgp30.hpp"

MHSGP30::MHSGP30(): SGP30(){
  co2_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) +"/SGP30-CO2/config";
  voc_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/SGP30-VOC/config";
}

bool MHSGP30::begin(){
  sensorReady=SGP30::begin();
  return sensorReady;
}

bool MHSGP30::available(){
  bool returnValue=true;

  if (sensorReady){
    if (isnan(CO2) || CO2== 0 || isnan(TVOC)){
      returnValue=false;
    }
  }else{
    returnValue=false;  
  }
  return returnValue;
}

void MHSGP30::read(){
    //read dht22 value
  measureAirQuality();
     
  Serial.print(CO2);
  Serial.print(" ");
  Serial.println(TVOC);
}

String MHSGP30::getDiscoveryMsg(String deviceName, deviceClass dev_class){
  String topic, unit, className;

  switch (dev_class){
    case temperature_sensor: unit = "ºC"; className="temperature"; topic= deviceName+"/DHT22/temperature"; break;
    case humidity_sensor: unit = "%"; className="humidity"; topic= deviceName+"/DHT22/humidity"; break;
    default: break;
  }

  return createDiscoveryMsg(topic, className, unit);
}
