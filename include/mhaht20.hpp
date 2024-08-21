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
#ifndef _METEOAHT20_
#define _METEOAHT20_

#include "meteosensor.hpp"
#include "meteoboard.hpp"
#include <Adafruit_AHTX0.h>

class MHAHT20: public MeteoSensor, public Adafruit_AHTX0{
  public:
    MHAHT20( MeteoBoard *p, Manager *m);
    bool available(); //! Detect if the device is available/connected to the board
    void read(); //! Read the values provided by de sensor
    String getDiscoveryMsg(String deviceName, deviceClass dev_class); //! Returns a Json with the complete discovery message
    void autodiscover(); //! Send autodiscovery messages to Home Assistant

    bool begin(); //! Redefinition/override of the begin function

    float getTemperature(){return temperature;}
    float getHumidity(){return humidity;}

    String getTemperatureDiscoveryTopic(){return temperature_discovery_topic;}
    String getHumidityDiscoveryTopic(){return humidity_discovery_topic;}

    String getTemperatureTopic(){return temperature_topic;}
    String getHumidityTopic(){return humidity_topic;}

  private:
    bool sensorReady;
    sensors_event_t humidity_event, temperature_event;
    float temperature = 0;
    float humidity = 0;

    String temperature_discovery_topic;
    String humidity_discovery_topic;

    String humidity_topic;
    String temperature_topic;
};
#endif