#ifndef _MANAGER_
#define _MANAGER_
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"      

#include <ArduinoJson.h> 
#include <FS.h> 

//Deep sleep
#define DEEP_SLEEP_TIME 60 //time in seconds


extern bool shouldSaveConfig;//flag for saving data

class Manager{

public:  
  explicit Manager();
  void setup_config_data();
  void setup_wifi();


  bool configFileExists;

  //callback notifying us of the need to save config
  static void saveConfigCallback () { Serial.println("Should save config"); shouldSaveConfig = true;}


  //MQTT  server
  char mqtt_server[15];
  char mqtt_port[6] ;
  char mqtt_user[30];
  char mqtt_password[30];

  //MQTT subscriptions
  char dht_temperature_topic[40];
  char dht_humidity_topic[40];
  char dht_heatindex_topic[40];
  char bmp_pressure_topic[40];
  char bmp_temperature_topic[40];

  
};

#endif
