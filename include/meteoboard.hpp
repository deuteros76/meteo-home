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
#ifndef _METEOBOARD_
#define _METEOBOARD_

#include <Arduino.h>
#include <ArduinoJson.h> 
#include <vector>
#include "meteosensor.hpp"
#include "manager.hpp"

class MeteoSensor;

/**
 * @brief A meteoboard is a developer board (mmicrocontroller). Many boad include their own sensors
 * so this class inherits MeteoSensor
 * 
 */
class MeteoBoard: public EspClass{
  public:
    MeteoBoard(Manager *m, PubSubClient *c);
    bool begin(); //! Redefinition/override of the begin function

    void autodiscover(); //! Send autodiscovery messages to Home Assistant
    

    template<class T> void addSensor(T *sensor){sensors.emplace_back(sensor);}
    void processSensors();

    bool connectToMQTT(); //! Connect to mqtt server
    PubSubClient *getClient(){return client;}
    void sendDiscoveryMessage(String discoveryTopic, String message);

  private:    
    Manager *manager;
    PubSubClient *client;
    
    std::vector<std::unique_ptr<MeteoSensor>> sensors; // To store sensors addresses
};

#endif