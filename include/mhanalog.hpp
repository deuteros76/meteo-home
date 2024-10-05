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

#ifndef _METEOANALOG_
#define _METEOANALOG_

#include "meteosensor.hpp"
#include <typeinfo>

class MHAnalog: public MeteoSensor{
  public:
    MHAnalog(MeteoBoard *p, Manager *m);
    bool available(); //! Detect if the device is available/connected to the board
    void read(); //! Read the values provided by de sensor
    String getDiscoveryMsg(String deviceName, deviceClass dev_class); //! Returns a Json with the complete discovery message
    void autodiscover(); //! Send autodiscovery messages to Home Assistant
         
    bool begin(); //! Redefinition/override of the begin function

    int getValue(){return value;}

    String getValueDiscoveryTopic(){return value_discovery_topic;}
    
    String getValueTopic(){return value_topic;}

  private:
    int value = 0;

    String value_discovery_topic;
    String value_topic;

    bool values_read; //! Flag to check if the sensr has been recently read. Used to force available function to read new values if the device is not available.

};
#endif
