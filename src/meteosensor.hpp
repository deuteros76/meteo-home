/*
Copyright 2022 meteo-home

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
#ifndef _METEOSENSOR_
#define _METEOSENSOR_

#include <Arduino.h>
#include <ArduinoJson.h> 
#include "manager.h"

extern Manager manager;

class MeteoSensor{
  public:
    enum deviceClass{temperature_sensor, humidity_sensor, pressure_sensor, co2_sensor, voc_sensor, voltage_sensor};
    
    virtual bool available() = 0; //! Detect if the device is available/connected to the board
    virtual void read() = 0; //! Read the values provided by de sensor
    virtual String getDiscoveryMsg(String deviceName, deviceClass dev_class) = 0; //! Returns a Json with the complete discovery message

  protected:
    String createDiscoveryMsg(String topic, String dev_class, String unit) ; //! Builds a JSON string containing the discovery message for Home Assistant

  private:
    String discoveryMessage;
};

#endif