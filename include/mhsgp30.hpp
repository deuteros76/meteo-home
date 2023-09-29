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

#ifndef _METEOSGP30_
#define _METEOSGP30_

#include "meteosensor.hpp"
#include <FS.h> 
#include <SparkFun_SGP30_Arduino_Library.h>

class MHSGP30: public MeteoSensor, public SGP30
{
  public:
    MHSGP30();
    bool available(); //! Detect if the device is available/connected to the board
    void read(); //! Read the values provided by de sensor
    void read(float temperature, float humidity); //! Read the values taking into accout current temperatura and humidity (better accuracy)
    String getDiscoveryMsg(String deviceName, deviceClass dev_class); //! Returns a Json with the complete discovery message
    void autodiscover(); //! Send autodiscovery messages to Home Assistant
    
    bool begin(TwoWire &wirePort = Wire); //! Redefinition/override of the begin function

    float getCO2(){return CO2;}
    float getVOC(){return TVOC;}

    String getCO2DiscoveryTopic(){return co2_discovery_topic;}
    String getVOCDiscoveryTopic(){return voc_discovery_topic;}

    String getCO2Topic(){return co2_topic;}
    String getVOCTopic(){return voc_topic;}

  private:
    bool sensorReady;

    String co2_discovery_topic;
    String voc_discovery_topic;

    String co2_topic;
    String voc_topic;  

    void readBaseline();
    void saveBaseline();

    double RHtoAbsolute (float relHumidity, float tempC); //! Relative humidity to absolute
    uint16_t doubleToFixedPoint( double number);

};
#endif
