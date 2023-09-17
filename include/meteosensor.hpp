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
#include <PubSubClient.h>
#include "manager.h"

extern Manager manager;

extern PubSubClient client;

class MeteoSensor{
  public:
    enum deviceClass{temperature_sensor, humidity_sensor, pressure_sensor, co2_sensor, voc_sensor, voltage_sensor};
    
    virtual bool available() = 0; //! Detect if the device is available/connected to the board
    virtual void read() = 0; //! Read the values provided by de sensor
    virtual String getDiscoveryMsg(String deviceName, deviceClass dev_class) = 0; //! Returns a Json with the complete discovery message
    virtual void autodiscover() = 0; //! Send autodiscovery messages to Home Assistant
    
    /**
     * @brief Creates a discovery message string. Builds a JSON string containing the discovery message for Home Assistant. 
     * These messages are used by Home Assistant to auto discover new MQTT topics
     * 
     * @param topic usually is the name of the device or the location/room where is installed
     * @param dev_class Home Assistant device class
     * @param unit unit of measurement
     * @return String the message to be sent to Home Assistant for autodiscovery
     */
    String createDiscoveryMsg(String topic, String dev_class, String unit) ; 

    void sendDiscoveryMessage(String discoveryTopic, String message);
  private:
    String discoveryMessage;

    bool connectToMQTT(); //! Connect to mqtt server
};

#endif