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
#ifndef _METEOBMP_
#define _METEOBMP_

#include "meteosensor.hpp"
#include <Adafruit_BMP085.h>

class MHBMP: MeteoSensor, public Adafruit_BMP085{
  public:
    MHBMP();
    bool available(); //! Detect if the device is available/connected to the board
    void read(); //! Read the values provided by de sensor
    String getDiscoveryMsg(String deviceName, deviceClass dev_class); //! Returns a Json with the complete discovery message

    bool begin(); //! Redefinition/override of the begin function

    float getTemperature(){return temperature;}
    float getPressure(){return pressure;}

    String getTemperatureDiscoveryTopic(){return temperature_discovery_topic;}
    String getPressureDiscoveryTopic(){return pressure_discovery_topic;}

  private:
    bool sensorReady;
    float temperature = 0;
    float pressure = 0;

    String temperature_discovery_topic;
    String pressure_discovery_topic;
};
#endif