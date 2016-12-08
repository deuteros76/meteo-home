#ifndef _MANAGER_
#define _MANAGER_
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"      

#include <ArduinoJson.h> 
#include <FS.h> 

//Deep sleep
#define DEEP_SLEEP_TIME 60 //time in seconds

#define ARDUINOJSON_ENABLE_STD_STREAM

using namespace std;

extern bool shouldSaveConfig;//flag for saving data

class Manager{

public:  
  explicit Manager();
  void setup_config_data();
  void setup_wifi();

  bool configFileExists;

  //callback notifying us of the need to save config
  static void saveConfigCallback () { Serial.println("Should save config"); shouldSaveConfig = true;}

  string mqttServer(){return mqtt_server;}
  string mqttPort(){return mqtt_port;}
  string mqttUser(){return mqtt_user;}
  string mqttPassword(){return mqtt_password;}

  string dhtTemperatureTopic(){return dht_temperature_topic;}
  string dhtHumidityTopic(){return dht_humidity_topic;}
  string dhtHeatindexTopic(){return dht_heatindex_topic;}
  string bmpPressureTopic(){return bmp_pressure_topic;}
  string bmpTemperatureTopic(){return bmp_temperature_topic;}

private:
  //MQTT  server
  string mqtt_server;
  string mqtt_port ;
  string mqtt_user;
  string mqtt_password;

  //MQTT subscriptions
  string dht_temperature_topic;
  string dht_humidity_topic;
  string dht_heatindex_topic;
  string bmp_pressure_topic;
  string bmp_temperature_topic;

  
};

#endif
