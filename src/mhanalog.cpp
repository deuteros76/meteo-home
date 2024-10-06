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

#include "mhanalog.hpp"

MHAnalog::MHAnalog(MeteoBoard *p, Manager *m){
  manager = m;
  parent = p;
  value_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) +"/Analog-value/config";

  value=0;
  values_read = false;
}

bool MHAnalog::begin(){
  bool returnValue=true;

  if (manager == nullptr){
    returnValue = false;
  }else if (available()){
    value_topic = manager->deviceName() + "/Analog/value";
  }else{
    returnValue=false;
  }
  return returnValue;
}

bool MHAnalog::available(){
  bool returnValue=true;

  //! A0 will always return a value between 0 and 1024 so the return value only depends on the configuration parameter useAnalogSensor
  manager->useAnalogSensor().toLowerCase();
  if (!manager->useAnalogSensor().equals("true")){
    returnValue= false;
  }

  if (!returnValue){
    Serial.println("[Analog] Error initializing Analog sensor.");
  }

  return returnValue;
}

void MHAnalog::read(){
   //read Analog value
  int rawValue=analogRead(A0);
  int minValue = manager->analogMinValue().toInt();
  int maxValue = manager->analogMaxValue().toInt();

  if (!values_read && (minValue>0 && maxValue>0)){   
    if ((minValue<=rawValue) && (rawValue<=maxValue)) {
      value = map(rawValue, maxValue, minValue, 0, 100);  
    }else{
      value = -1;
    }
  }

  parent->getClient()->publish(getValueTopic().c_str(), String(getValue()).c_str(), true);
  delay(50);
     
  Serial.println("[Analog] Value = " + String(getValue()) + "% ("+String(rawValue)+")");
  values_read= false;
}

String MHAnalog::getDiscoveryMsg(String deviceName, deviceClass dev_class){
  String topic, unit, className;

  switch (dev_class){
    case analog_sensor: unit = "%"; className="moisture"; topic= deviceName+"/Analog/value"; break;
    default: break;
  }

  return createDiscoveryMsg(topic, className, unit);
}

void MHAnalog::autodiscover(){
  if (available()){
     parent->sendDiscoveryMessage(getValueDiscoveryTopic(), getDiscoveryMsg(manager->deviceName(),MeteoSensor::deviceClass::analog_sensor));    
  }
}
