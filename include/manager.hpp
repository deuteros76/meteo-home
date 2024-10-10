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

#ifndef _MANAGER_
#define _MANAGER_
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"      

#include <ArduinoJson.h> 
#include <LittleFS.h> 

//Deep sleep
#define WIFI_CONNECTION_TIMEOUT 20000 //Timeout for WIFI connections. The idea is to prevent for continuous conection tries. This would cause battery drain

#ifndef ARDUINOJSON_ENABLE_STD_STREAM
#define ARDUINOJSON_ENABLE_STD_STREAM
#endif

//using namespace std;

extern bool shouldSaveConfig;//flag for saving data

class Manager{

public:  
  explicit Manager();
  void setup_config_data();
  void setup_wifi();

  bool configFileExists;

  //callback notifying us of the need to save config
  static void saveConfigCallback () { Serial.println("[Manager] Callback. Should save config"); }

  //Make the device discoverable
  //String getDiscoveryMsg(String topic, device_class dev_class);

  String networkIp(){return network_ip;}
  String networkMask(){return network_mask;}
  String networkGateway(){return network_gateway;}

  String mqttServer(){return mqtt_server;}
  String mqttPort(){return mqtt_port;}
  String mqttUser(){return mqtt_user;}
  String mqttPassword(){return mqtt_password;}
  
  String useSleepMode(){return use_sleep_mode;}  
  int sleepMinutes(){if (sleep_minutes<1) return 1; else return sleep_minutes;}  
  String deviceName(){return device_name;}

  bool useAnalogSensor(){return use_analog_sensor;} 
  String sensorClass(){return sensor_class;} 
  bool useArduinoMapFunction(){return use_arduino_map_function;}
  String analogMinValue(){return analog_min_value;}
  String analogMaxValue(){return analog_max_value;}
  
private:
  //MQTT  server
  String network_ip;
  String network_mask ;
  String network_gateway;
  
  //MQTT  server
  String mqtt_server;
  String mqtt_port ;
  String mqtt_user;
  String mqtt_password;
  
  String use_sleep_mode;
  int sleep_minutes;

  bool use_analog_sensor;
  String sensor_class;
  bool use_arduino_map_function;
  String analog_min_value;
  String analog_max_value;

  String device_name; //! Device (or location) name used to generate the MQTT topics

};

#endif
