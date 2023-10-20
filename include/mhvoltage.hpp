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
#ifndef _METEOVOLTAGE_
#define _METEOVOLTAGE_

#include "meteosensor.hpp"

class MHVoltage: public MeteoSensor, public EspClass{
  public:
    MHVoltage(MeteoBoard *p, Manager *m);
    bool available(); //! Detect if the device is available/connected to the board
    void read(); //! Read the values provided by de sensor
    String getDiscoveryMsg(String deviceName, deviceClass dev_class); //! Returns a Json with the complete discovery message
    void autodiscover(); //! Send autodiscovery messages to Home Assistant

    bool begin(); //! Redefinition/override of the begin function

    float getVoltage(){return voltage;}

    String getVoltageDiscoveryTopic(){return voltage_discovery_topic;}

    String getVoltageTopic(){return voltage_topic;}

  private:
    bool sensorReady;
    float voltage = 0;

    String voltage_discovery_topic;

    String voltage_topic;
};
#endif