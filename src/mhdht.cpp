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

MHDHT::MHDHT(Manager *m, uint8_t pin, uint8_t type): DHT(pin, type){
  manager = m;
  temperature_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) +"/DHT22-temperature/config";
  humidity_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/DHT22-humidity/config";
  heatindex_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/DHT22-heatindex/config";

  values_read = false;
}

bool MHDHT::begin(){
  bool returnValue=true;

  DHT::begin();
  delay(50);
  read(); // this reading is to make available() work from the begining

  if (manager == nullptr){
    returnValue = false;
  }else if (available()){
    temperature_topic = manager->deviceName() + "/DHT22/temperature";
    humidity_topic = manager->deviceName() + "/DHT22/humidity";
    heatindex_topic = manager->deviceName() + "/DHT22/heatindex";
  }else{
    returnValue=false;
  }
  return returnValue; //! TODO: think about this boolean functio
}

bool MHDHT::available(){
  bool returnValue=true;

  if (isnan(temperature) || isnan(humidity)){
    if (values_read){
      returnValue=false;
    }else{
      values_read= true;
      returnValue = (!isnan(temperature) && !isnan(humidity));
    }

  }
  return returnValue;
}

void MHDHT::read(){
    //read dht22 value
  if (!values_read){
    temperature = readTemperature();    
    humidity = readHumidity();
    heatindex = computeHeatIndex(temperature, humidity, false);
  }

  client.publish(getTemperatureTopic().c_str(), String(getTemperature()).c_str(), true);
  delay(50);
  client.publish(getHumidityTopic().c_str(), String(getHumidity()).c_str(), true);
  delay(50);
  client.publish(getHeatindexTopic().c_str(), String(getHeatIndex()).c_str(), true);
  delay(50);
     
  Serial.println("[DHT] Temperature = " + String(temperature) + " Humidity = " + String(humidity) +" HeatIndex = " + String(heatindex));
  values_read= false;
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

void MHDHT::autodiscover(){
  if (available()){
      sendDiscoveryMessage(getTemperatureDiscoveryTopic(), getDiscoveryMsg(manager->deviceName(),MeteoSensor::deviceClass::temperature_sensor));    
      sendDiscoveryMessage(getHumidityDiscoveryTopic(), getDiscoveryMsg(manager->deviceName(), MeteoSensor::deviceClass::humidity_sensor));
  }
}
