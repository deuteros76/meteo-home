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

#ifndef _METEODHT_
#define _METEODHT_

#include "meteosensor.hpp"
#include <DHT.h>

class MHDHT: MeteoSensor, public DHT{
  public:
    MHDHT(uint8_t pin, uint8_t type);
    bool available(); //! Detect if the device is available/connected to the board
    void read(); //! Read the values provided by de sensor
    String getDiscoveryMsg(String deviceName, deviceClass dev_class); //! Returns a Json with the complete discovery message

    float getTemperature(){return temperature;}
    float getHumidity(){return humidity;}
    float getHeatIndex(){return heatindex;}

    String getTemperatureDiscoveryTopic(){return temperature_discovery_topic;}
    String getHumidityDiscoveryTopic(){return humidity_discovery_topic;}
    String getHeatindexDiscoveryTopic(){return heatindex_discovery_topic;}
    
    String getTemperatureTopic(){return temperature_topic;}
    String getHumidityTopic(){return humidity_topic;}
    String getHeatindexTopic(){return heatindex_topic;}  

  private:
    float temperature = 0;
    float humidity = 0;
    float heatindex = 0;

    String temperature_discovery_topic;
    String humidity_discovery_topic;
    String heatindex_discovery_topic;

    String temperature_topic;
    String humidity_topic;
    String heatindex_topic;

};
#endif
