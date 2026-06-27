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

MHDHT::MHDHT(MeteoBoard *p, Manager *m, uint8_t pin, uint8_t type): DHT(pin, type){
  manager = m;
  parent = p;
  temperature_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) +"/DHT22-temperature/config";
  humidity_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/DHT22-humidity/config";
  heatindex_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/DHT22-heatindex/config";
}

bool MHDHT::begin(){
  bool returnValue=true;

  DHT::begin();
  delay(50);

  if (manager == nullptr){
    returnValue = false;
  }else if (available()){ // available() performs a fresh read and validates it
    temperature_topic = manager->deviceName() + "/DHT22/temperature";
    humidity_topic = manager->deviceName() + "/DHT22/humidity";
    heatindex_topic = manager->deviceName() + "/DHT22/heatindex";
  }else{
    returnValue=false;
  }
  return returnValue; //! TODO: think about this boolean functio
}

bool MHDHT::available(){
  // Take a fresh measurement. The DHT is a 1-wire sensor, so a failure shows up
  // as NaN readings. read() publishes these values without re-reading, so a
  // failing sensor stops publishing and HA marks it unavailable via expire_after.
  temperature = readTemperature();
  humidity = readHumidity();
  return (!isnan(temperature) && !isnan(humidity));
}

void MHDHT::read(){
  heatindex = computeHeatIndex(temperature, humidity, false);

  parent->getClient()->publish(getTemperatureTopic().c_str(), String(getTemperature()).c_str(), true);
  delay(50);
  parent->getClient()->publish(getHumidityTopic().c_str(), String(getHumidity()).c_str(), true);
  delay(50);
  parent->getClient()->publish(getHeatindexTopic().c_str(), String(getHeatIndex()).c_str(), true);
  delay(50);

  Serial.println("[DHT] Temperature = " + String(temperature) + " Humidity = " + String(humidity) +" HeatIndex = " + String(heatindex));
}

String MHDHT::getDiscoveryMsg(String deviceName, deviceClass dev_class){
  String topic, unit, className;

  switch (dev_class){
    case temperature_sensor: unit = "°C"; className="temperature"; topic= deviceName+"/DHT22/temperature"; break;
    case humidity_sensor: unit = "%"; className="humidity"; topic= deviceName+"/DHT22/humidity"; break;
    default: break;
  }

  return createDiscoveryMsg(topic, className, unit);
}

void MHDHT::autodiscover(){
  if (available()){
     parent->sendDiscoveryMessage(getTemperatureDiscoveryTopic(), getDiscoveryMsg(manager->deviceName(),MeteoSensor::deviceClass::temperature_sensor));    
     parent->sendDiscoveryMessage(getHumidityDiscoveryTopic(), getDiscoveryMsg(manager->deviceName(), MeteoSensor::deviceClass::humidity_sensor));
  }
}
