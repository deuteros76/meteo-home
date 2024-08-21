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

#include "mhaht20.hpp"

MHAHT20::MHAHT20( MeteoBoard *p, Manager *m): Adafruit_AHTX0(){
  manager = m;
  parent = p;
  temperature_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) +"/AHT20-temperature/config";
  humidity_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/AHT20-humidity/config";

  sensorReady=false;
}

bool MHAHT20::begin(){
  if (manager == nullptr){
    sensorReady = false;
  }else{
    sensorReady=Adafruit_AHTX0::begin();
    
    humidity_topic = manager->deviceName() + "/AHT20/humidity";
    temperature_topic = manager->deviceName() + "/AHT20/temperature";
  }
  if (!sensorReady){
    Serial.println("[AHT20] Error initializing AHT20 sensor.");
  }
  return sensorReady;
}

bool MHAHT20::available(){
  bool returnValue=true;

  if (!sensorReady){   
    returnValue=false;  
  }
  return returnValue;
}

void MHAHT20::read(){
    //read dht22 value
  if (sensorReady){
    getEvent(&humidity_event, &temperature_event);
    temperature = temperature_event.temperature; 
    humidity = humidity_event.relative_humidity; //Dividing by 100 we get The humidity in Bars

    parent->getClient()->publish(getHumidityTopic().c_str(), String(getHumidity()).c_str(), true); 
    delay(50);
    parent->getClient()->publish(getTemperatureTopic().c_str(), String(getTemperature()).c_str(), true); 
    delay(50);

    Serial.println("[AHT20] Temperature = " + String(temperature) + " Humidity = " + String(humidity));
  }
}

String MHAHT20::getDiscoveryMsg(String deviceName, deviceClass dev_class){
  String topic, unit, className;

  switch (dev_class){
    case MeteoSensor::temperature_sensor: unit = "ºC"; className="temperature"; topic= deviceName+"/AHT20/temperature"; break;
    case MeteoSensor::humidity_sensor: unit = "Pa"; className="atmospheric_humidity"; topic= deviceName+"/AHT20/humidity"; break;
    default: break;
  }

  return createDiscoveryMsg(topic, className, unit);
}

void MHAHT20::autodiscover(){
  if (available()){
      parent->sendDiscoveryMessage(getTemperatureDiscoveryTopic(), getDiscoveryMsg(manager->deviceName(),MeteoSensor::deviceClass::temperature_sensor));
      parent->sendDiscoveryMessage(getHumidityDiscoveryTopic(), getDiscoveryMsg(manager->deviceName(), MeteoSensor::deviceClass::humidity_sensor));
   }
}