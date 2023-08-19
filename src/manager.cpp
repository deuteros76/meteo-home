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

#include "manager.h"
#include <string>

/*
template class std::basic_string<char>;
*/
bool shouldSaveConfig=false;

Manager::Manager(){ 
  
  network_ip = "192.168.1.206";
  network_mask = "255.255.255.0";
  network_gateway = "192.168.1.1";
  
  mqtt_server = "192.168.1.100";
  mqtt_port = "1883";
  mqtt_user = "your_username";
  mqtt_password = "YOUR_PASSWORD";
  
  device_name = "Terrace"; 

  //flag for saving data
  shouldSaveConfig = false;
  configFileExists = false;
}

void Manager::setup_config_data(){
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    //SPIFFS.remove("/config.json");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(1024);
        deserializeJson(json,buf.get());
        serializeJson(json, Serial);
        if (!json.isNull()) {
          configFileExists=true;
          Serial.println("\nparsed json:");
          
          serializeJson(json, Serial);Serial.println();

          network_ip = (const char *)json["network_ip"];
          network_mask = (const char *)json["network_mask"];
          network_gateway = (const char *)json["network_gateway"];

          mqtt_server = (const char *)json["mqtt_server"];
          mqtt_port = (const char *)json["mqtt_port"];
          mqtt_user = (const char *)json["mqtt_user"];
          mqtt_password = (const char *)json["mqtt_password"];
          
          use_sleep_mode = (const char *)json["use_sleep_mode"];
          device_name = (const char *)json["device_name"];

          dht_temperature_topic = (const char *)json["dht_temperature_topic"];
          dht_humidity_topic = (const char *)json["dht_humidity_topic"];
          dht_heatindex_topic = (const char *)json["dht_heatindex_topic"];
          
          bmp_pressure_topic = (const char *)json["bmp_pressure_topic"];
          bmp_temperature_topic=(const char *)json["bmp_temperature_topic"];     

        } else {
          Serial.println("failed to load json config");
        }
      }
    }else{
      shouldSaveConfig=true;
    }
  } else {
    Serial.println("failed to mount FS");
  }
}


void Manager::setup_wifi(){
  WiFiManagerParameter custom_network_group("<p>Network settings</p>");
  WiFiManagerParameter custom_network_ip("IP", "IP", network_ip.c_str(), 15);
  WiFiManagerParameter custom_network_mask("mask", "mask", network_mask.c_str(), 15);
  WiFiManagerParameter custom_network_gateway("gateway", "gateway", network_gateway.c_str(), 15);
  
  WiFiManagerParameter custom_server_group("<p>MQTT Sercer settings</p>");
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server.c_str(), 15);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port.c_str(), 6);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password.c_str(), 30);
  WiFiManagerParameter custom_mqtt_username("username", "user name", mqtt_user.c_str(), 30);
   
  WiFiManagerParameter custom_paramenters_group("<p>Device parameters</p>");
  WiFiManagerParameter custom_device_name("name","device name or location",device_name.c_str(),40);
  const char* custom_sleepmode_checkbox_str = "type='checkbox'";
  WiFiManagerParameter custom_use_sleep_mode("sleepmode", "Sleep mode", "true", 4,custom_sleepmode_checkbox_str,WFM_LABEL_AFTER);

  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(&Manager::saveConfigCallback);
  wifiManager.setBreakAfterConfig(true); //https://github.com/tzapu/WiFiManager/issues/992 IF YOU NEED TO SAVE PARAMETERS EVEN ON WIFI FAIL OR EMPTY, you must set setBreakAfterConfig to true, or else saveConfigCallback will not be called.

  //reset settings - for testing
  //wifiManager.resetSettings();
  wifiManager.addParameter(&custom_network_group);
  wifiManager.addParameter(&custom_network_ip);
  wifiManager.addParameter(&custom_network_mask);
  wifiManager.addParameter(&custom_network_gateway);
 
  wifiManager.addParameter(&custom_server_group);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  
  wifiManager.addParameter(&custom_paramenters_group);
  wifiManager.addParameter(&custom_device_name);
  wifiManager.addParameter(&custom_use_sleep_mode);

  long wifiTimeStart = millis();

  if (configFileExists){
    //read updated parameters
    IPAddress ip,gateway,mask;
    Serial.println(network_ip);
    Serial.println(network_gateway.c_str());
    Serial.println(network_mask.c_str());
    Serial.println(WiFi.macAddress());
    ip.fromString(network_ip.c_str());
    gateway.fromString(network_gateway.c_str());
    mask.fromString(network_mask.c_str());

    String hostname = "Meteo-home_";
    hostname.concat(WiFi.macAddress());
    WiFi.config(ip, gateway,mask);
    WiFi.hostname(hostname.c_str());
    WiFi.mode(WIFI_STA);
    while (WiFi.status() != WL_CONNECTED){
      wifiTimeStart = millis();
      if (strlen(WiFi.psk().c_str())==0){
        WiFi.begin(WiFi.SSID().c_str());
        Serial.printf("\nConnecting to an open network (%s).\n",WiFi.SSID().c_str());  
      }
      else {
        WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
        Serial.printf("\nConnecting to an encrypted network (%s).\n",WiFi.SSID().c_str());  
      }
      
      while (WiFi.status() != WL_CONNECTED && (millis() - wifiTimeStart < WIFI_CONNECTION_TIMEOUT)) {
        delay(500);
        Serial.print(".");
      }
      
      Serial.println("\nUnable to connect to the WiFi network. Trying again.");   
    }
    if (WiFi.status() != WL_CONNECTED){
      Serial.println("\nIt was unable to connect to the WiFi network. Going to sleep");
      ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);      
    }
    
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("Network configuration: ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.gatewayIP());
    Serial.println(WiFi.subnetMask());
    Serial.println(WiFi.hostname());
  }else {
        wifiManager.setTimeout(300);
        wifiManager.startConfigPortal("Meteo-home");
  }
  
  //if you get here you have connected to the WiFi
  Serial.println("Connected!!)");
    
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonDocument json(1024);

    json["network_ip"] = custom_network_ip.getValue();
    json["network_mask"] = custom_network_mask.getValue();
    json["network_gateway"] = custom_network_gateway.getValue();
    
    json["mqtt_server"] = custom_mqtt_server.getValue();
    json["mqtt_port"] = custom_mqtt_port.getValue();
    json["mqtt_user"] = custom_mqtt_username.getValue();
    json["mqtt_password"] = custom_mqtt_password.getValue();
    
    json["use_sleep_mode"] = custom_use_sleep_mode.getValue();
    json["device_name"] = custom_device_name.getValue();
    
    json["dht_temperature_topic"] = String(custom_device_name.getValue()) + "/DHT22/temperature";
    Serial.println(String(custom_device_name.getValue())+"/DHT22/temperature");
    Serial.println("=================");
    json["dht_humidity_topic"] = String(custom_device_name.getValue()) + "/DHT22/humidity";
    json["dht_heatindex_topic"] = String(custom_device_name.getValue()) + "/DHT22/heatindex";
    json["bmp_pressure_topic"] = String(custom_device_name.getValue()) + "/bmp180/pressure";
    json["bmp_temperature_topic"] = String(custom_device_name.getValue()) + "/bmp180/temperature";

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    serializeJson(json, Serial);    
    serializeJson(json, configFile);
    configFile.close();
    //end save
  }
 
}
