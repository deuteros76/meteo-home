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

#include "mhvoltage.hpp"

MHVoltage::MHVoltage(MeteoBoard *p, Manager *m){
  manager = m;
  parent = p;
  voltage_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/ESP-voltage/config";
}

bool MHVoltage::begin(){
  bool returnValue = false;
  if (manager != nullptr){
    voltage_topic = manager->deviceName() + "/ESP/vcc";
    returnValue = true;
  }

  return returnValue; //! TODO: think about this boolean function
}

bool MHVoltage::available(){
  bool returnValue=true;

   //! TODO: think about this boolean function
  return returnValue;
}

void MHVoltage::read(){
  voltage = getVcc()/1000.0;
  parent->getClient()->publish(getVoltageTopic().c_str(), String(getVoltage()).c_str(), true);    
  Serial.println("[Board] Voltage = " + String(voltage));
}

String MHVoltage::getDiscoveryMsg(String deviceName, deviceClass dev_class){
  String unit, className;

  unit = "V"; 
  className="voltage"; 
  return createDiscoveryMsg(voltage_topic, className, unit);
}

void MHVoltage::autodiscover(){
  if (available()){
    parent->sendDiscoveryMessage(getVoltageDiscoveryTopic(), getDiscoveryMsg(manager->deviceName(),MeteoSensor::deviceClass::voltage_sensor));
  }
}