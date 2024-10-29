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

/**
 * @class MHAnalog
 * @brief Represents an analog meteo sensor that can be used with the MeteoBoard.
 *
 * The MHAnalog class is a concrete implementation of the MeteoSensor interface. It provides methods to detect if the sensor is available, read its values, and generate discovery messages for Home Assistant.
 *
 * @param p A pointer to the MeteoBoard instance that the sensor is connected to.
 * @param m A pointer to the Manager instance that manages the sensor.
 *
 * @see MeteoSensor
 * @see MeteoBoard
 * @see Manager
 */
class MHAnalog: public MeteoSensor{
  public:
    /**
     * Constructs an MHAnalog instance.
     *
     * @param p A pointer to the MeteoBoard instance that the sensor is connected to.
     * @param m A pointer to the Manager instance that manages the sensor.
     */
    MHAnalog(MeteoBoard *p, Manager *m);
    
    /**
     * Detects if the device is available/connected to the board.
     *
     * @return true if the device is available, false otherwise.
     */
    bool available();
    
    /**
     * Reads the values provided by the sensor.
     */
    void read();
    
    /**
     * Returns a JSON with the complete discovery message.
     *
     * @param deviceName The name of the device.
     * @param dev_class The device class.
     * @return The discovery message.
     */
    String getDiscoveryMsg(String deviceName, deviceClass dev_class);
    
    /**
     * Sends autodiscovery messages to Home Assistant.
     */
    void autodiscover();
    
    /**
     * Redefines/overrides the begin function.
     *
     * @return true if the begin operation was successful, false otherwise.
     */
    bool begin();
    
    /**
     * Returns the current value of the sensor.
     *
     * @return The current value.
     */
    int getValue(){return value;}
    
    /**
     * Returns the value discovery topic.
     *
     * @return The value discovery topic.
     */
    String getValueDiscoveryTopic(){return value_discovery_topic;}
    
    /**
     * Returns the value topic.
     *
     * @return The value topic.
     */
     String getValueTopic(){return value_topic;}   

  private:
    int value = 0;

    String value_discovery_topic;
    String value_topic;

    bool values_read; //! Flag to check if the sensr has been recently read. Used to force available function to read new values if the device is not available.

};
#endif
