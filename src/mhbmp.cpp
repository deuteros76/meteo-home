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

#include "mhbmp.hpp"

MHBMP::MHBMP(): Adafruit_BMP085(){
  temperature_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) +"/BMP-temperature/config";
  pressure_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/BMP-pressure/config";
}

bool MHBMP::begin(){
  sensorReady=Adafruit_BMP085::begin();
  return sensorReady;
}

bool MHBMP::available(){
  bool returnValue=true;

  if (!sensorReady){   
    returnValue=false;  
  }
  return returnValue;
}

void MHBMP::read(){
    //read dht22 value
  if (sensorReady){
    temperature = readTemperature(); 
    pressure = readPressure()/100.0; //Dividing by 100 we get The pressure in Bars
      
    Serial.print(temperature);
    Serial.print(" ");
    Serial.println(pressure);
  }
}

String MHBMP::getDiscoveryMsg(String deviceName, deviceClass dev_class){
  String topic, unit, className;

  switch (dev_class){
    case temperature_sensor: unit = "ºC"; className="temperature"; topic= deviceName+"/BMP180/temperature"; break;
    case pressure_sensor: unit = "%"; className="atmospheric_pressure"; topic= deviceName+"/BMP180/pressure"; break;
    default: break;
  }

  return createDiscoveryMsg(topic, className, unit);
}
