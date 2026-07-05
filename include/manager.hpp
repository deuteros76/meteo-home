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
#include "WiFiManager.h"      

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <vector>

//Deep sleep
#define WIFI_CONNECTION_TIMEOUT 20000 //Timeout for WIFI connections. The idea is to prevent for continuous conection tries. This would cause battery drain
#define WIFI_MAX_CONNECTION_RETRIES 3 //Max WiFi connection attempts before giving up and deep sleeping. Prevents draining the battery (e.g. solar setups) by retrying the radio forever when it cannot connect

#ifndef ARDUINOJSON_ENABLE_STD_STREAM
#define ARDUINOJSON_ENABLE_STD_STREAM
#endif

//using namespace std;

extern bool shouldSaveConfig;//flag for saving data

//! Outcome of a remote (MQTT) configuration attempt via Manager::applyRemoteConfig.
struct RemoteConfigOutcome {
  bool success = false;
  String reason;                        // "applied", "invalid_token", "invalid_payload", "invalid_field:<name>"
  std::vector<String> appliedFields;     // solo relevante cuando success == true
};

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
  
  bool useSleepMode(){return use_sleep_mode;}  
  int sleepMinutes(){if (sleep_minutes<1) return 1; else return sleep_minutes;}  
  String deviceName(){return device_name;}

  //! Per-device MQTT availability (LWT) topic. Keeps each device's online/offline
  //! status independent so one device going offline does not mark the others.
  String availabilityTopic(){return "meteohome/" + device_name + "/status";}

  //! Topic where an external client publishes a retained remote-configuration command.
  String configSetTopic(){return "meteohome/" + device_name + "/config/set";}
  //! Topic where this device publishes the result of processing a configuration command.
  String configResultTopic(){return "meteohome/" + device_name + "/config/result";}

  bool useAnalogSensor(){return use_analog_sensor;} 
  String sensorClass(){return sensor_class;} 
  bool useArduinoMapFunction(){return use_arduino_map_function;}
  int analogMinValue(){return analog_min_value;}
  int analogMaxValue(){return analog_max_value;}
  
  String configToken(){return config_token;}

  // Setters used by remote config flow (Task 4) and by this test.
  void setDeviceName(String name){device_name = name;}
  void setUseSleepMode(bool value){use_sleep_mode = value;}
  void setSleepMinutes(int value){sleep_minutes = value;}
  void setUseAnalogSensor(bool value){use_analog_sensor = value;}
  void setSensorClass(String value){sensor_class = value;}
  void setUseArduinoMapFunction(bool value){use_arduino_map_function = value;}
  void setAnalogMinValue(int value){analog_min_value = value;}
  void setAnalogMaxValue(int value){analog_max_value = value;}

  //! Validates and applies a remote (MQTT) configuration payload. Transactional:
  //! either every present field is valid and gets applied, or nothing changes.
  RemoteConfigOutcome applyRemoteConfig(JsonDocument &doc);

#ifdef PIO_UNIT_TESTING
  void persistConfigForTest(){persistConfig();}
#endif

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
  
  bool use_sleep_mode;
  int sleep_minutes;

  bool use_analog_sensor;
  String sensor_class;
  bool use_arduino_map_function;
  int analog_min_value;
  int analog_max_value;

  String device_name; //! Device (or location) name used to generate the MQTT topics

  String config_token;

  void persistConfig(); //! Serializes all current members to /config.json
  String generateToken(); //! Generates a random 32-hex-char config token

};

#endif
