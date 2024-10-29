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

#include "manager.hpp"
#include <string>
#include <front/css.hpp>

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
  Serial.println("[Manager] Mounting FS...");

  if (LittleFS.begin()) {
    //LittleFS.remove("/config.json");
    if (LittleFS.exists("/config.json")) {
      //file exists, reading and loading
      File configFile = LittleFS.open("/config.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(1024);
        deserializeJson(json,buf.get());
        serializeJson(json, Serial);
        if (!json.isNull()) {
          configFileExists=true;
          Serial.println("\n[Manager] Parsed json:");
          
          serializeJson(json, Serial);Serial.println();

          network_ip = (const char *)json["network_ip"];
          network_mask = (const char *)json["network_mask"];
          network_gateway = (const char *)json["network_gateway"];

          mqtt_server = (const char *)json["mqtt_server"];
          mqtt_port = (const char *)json["mqtt_port"];
          mqtt_user = (const char *)json["mqtt_user"];
          mqtt_password = (const char *)json["mqtt_password"];
          
          String aux = (const char *)json["use_sleep_mode"];
          aux.toLowerCase();
          use_sleep_mode = aux.equals("true");
          sleep_minutes= String((const char *)json["sleep_minutes"]).toInt();
          device_name = (const char *)json["device_name"];

          aux = (const char *)json["use_analog_sensor"];
          aux.toLowerCase();
          use_analog_sensor = aux.equals("true");
          sensor_class = (const char *)json["sensor_class"];
          String map_function=(const char *)json["use_arduino_map_function"];
          map_function.toLowerCase();
          use_arduino_map_function = map_function.equals("true");
          analog_min_value = String((const char *)json["analog_min_value"]).toInt();
          analog_max_value = String((const char *)json["analog_max_value"]).toInt();

        } else {
          Serial.println("[Manager] Failed to load json config");
        }
      }
    }else{
      shouldSaveConfig=true;
      Serial.println("[Manager] Config file not found.");
    }
  } else {
    Serial.println("[Manager] Failed to mount FS");
  }
}

void Manager::setup_wifi(){
  WiFiManagerParameter custom_show_hide_function= "<script>function changeVisibility(element) {var x = document.getElementById(element);if (x.style.display === \"none\") {    x.style.display = \"block\";  } else {    x.style.display = \"none\";}}</script>";
  WiFiManagerParameter custom_network_group("<div class='four'><h1>Network settings</h1></div>");
  WiFiManagerParameter custom_network_ip("IP", "IP", network_ip.c_str(), 15);
  WiFiManagerParameter custom_network_mask("mask", "Mask", network_mask.c_str(), 15);
  WiFiManagerParameter custom_network_gateway("gateway", "Gateway", network_gateway.c_str(), 15);
  
  
  WiFiManagerParameter custom_panel("<div class='panel'>");
  WiFiManagerParameter custom_server_group("<div class='four'><h1>MQTT Server settings</h1></div>");
  WiFiManagerParameter custom_mqtt_server("server", "MQTT server", mqtt_server.c_str(), 15);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT port", mqtt_port.c_str(), 6);
  WiFiManagerParameter custom_mqtt_password("password", "MQTT password", mqtt_password.c_str(), 30);
  WiFiManagerParameter custom_mqtt_username("username", "User name", mqtt_user.c_str(), 30);
  WiFiManagerParameter custom_panel_end("</div>");

  WiFiManagerParameter custom_paramenters_group("<div class='four'><h1>Device parameters</h1></div>");
  WiFiManagerParameter custom_device_name("name","Device name or location",device_name.c_str(),40);
  
  const char* custom_sleepmode_checkbox_str = "type='checkbox'";  
  WiFiManagerParameter custom_use_sleep_mode("sleepmode", "Use deepsleep mode (for battery powered projects)", "true", 5,custom_sleepmode_checkbox_str, WFM_LABEL_AFTER);
  WiFiManagerParameter custom_sleep_group("<div id='sleepgrp'>");
  WiFiManagerParameter custom_sleep_minutes_lbl ("<br/><label for='minutes'>Minutes between data gathering: </label><output id='outminutes'>1</output>");
  const char* custom_sleep_minutes_str="type='range'  value='1' step='1' min='1' max='60' oninput=\"document.getElementById('outminutes').value = this.value;\"";  
  WiFiManagerParameter custom_sleep_minutes("minutes","","1",2,custom_sleep_minutes_str,WFM_NO_LABEL);
  WiFiManagerParameter custom_sleep_group_end("</div>");  

  const char* custom_analogsensor_checkbox_str = "type='checkbox' onchange=\"changeVisibility('analogconfig');\"";
  WiFiManagerParameter custom_use_analog_sensor("analogsensor", "Read analog input (when an analog sensor is connected)" , "true", 5,custom_analogsensor_checkbox_str, WFM_LABEL_AFTER);
  WiFiManagerParameter custom_analog_config_group("<div id='analogconfig' style=\"display:none\">");
  WiFiManagerParameter custom_sensor_class("sensorclass", "Sensor class name (see <a href=\"https://developers.home-assistant.io/docs/core/entity/sensor/#available-device-classes\">HA device classes</a>)","moisture",20);
  const char* custom_use_map_function_str = "type='checkbox' onchange=\"changeVisibility('analoggrp'); document.getElementById('minvalue').disabled = !this.checked; document.getElementById('maxvalue').disabled = !this.checked;\"";
  WiFiManagerParameter custom_use_map_function("mapfunction", "Use Arduino map function to scale values between 0 and 100%","true",5,custom_use_map_function_str,WFM_LABEL_AFTER);
  WiFiManagerParameter custom_analog_group("<div id='analoggrp' style=\"display:none\">");
  WiFiManagerParameter custom_analog_min_value_lbl ("<br/><label for='minvalue'>Min value (used for 0%): </label><output id='outminvalue'>0</output>");
  const char* custom_analog_min_value_str = "type='range' disabled step='1' min='0' max='1024' oninput=\"document.getElementById('outminvalue').value = this.value;\"";  
  WiFiManagerParameter custom_analog_min_value("minvalue","","0",5,custom_analog_min_value_str,WFM_NO_LABEL);  
  WiFiManagerParameter custom_analog_max_value_lbl ("<br/><label for='maxvalue'>Max value (used for 100%): </label><output id='outmaxvalue'>1024</output>");
  const char* custom_analog_max_value_str = "type='range' disabled step='1' min='0' max='1024' oninput=\"document.getElementById('outmaxvalue').value = this.value;\"";  
  WiFiManagerParameter custom_analog_max_value("maxvalue", "","1024",5,custom_analog_max_value_str,WFM_NO_LABEL);
  WiFiManagerParameter custom_analog_group_end("</div>");  
  WiFiManagerParameter custom_analog_config_group_end("</div>");  

  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(&Manager::saveConfigCallback);
  wifiManager.setBreakAfterConfig(true); //https://github.com/tzapu/WiFiManager/issues/992 IF YOU NEED TO SAVE PARAMETERS EVEN ON WIFI FAIL OR EMPTY, you must set setBreakAfterConfig to true, or else saveConfigCallback will not be called.

  //reset settings - for testing
  //wifiManager.resetSettings();
  wifiManager.setCustomHeadElement(CSS_CODE.c_str());

  wifiManager.addParameter(&custom_show_hide_function);
  wifiManager.addParameter(&custom_panel);
  wifiManager.addParameter(&custom_network_group);
  wifiManager.addParameter(&custom_network_ip);
  wifiManager.addParameter(&custom_network_mask);
  wifiManager.addParameter(&custom_network_gateway);
  wifiManager.addParameter(&custom_panel_end);
 
  wifiManager.addParameter(&custom_panel);
  wifiManager.addParameter(&custom_server_group);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_panel_end);  

  wifiManager.addParameter(&custom_panel);
  wifiManager.addParameter(&custom_paramenters_group);
  wifiManager.addParameter(&custom_device_name);
  
  wifiManager.addParameter(&custom_use_sleep_mode);
  wifiManager.addParameter(&custom_sleep_group);
  wifiManager.addParameter(&custom_sleep_minutes_lbl);
  wifiManager.addParameter(&custom_sleep_minutes);
  wifiManager.addParameter(&custom_sleep_group_end);

  wifiManager.addParameter(&custom_use_analog_sensor);
  wifiManager.addParameter(&custom_analog_config_group);
  wifiManager.addParameter(&custom_sensor_class);
  wifiManager.addParameter(&custom_use_map_function);
  wifiManager.addParameter(&custom_analog_group);
  wifiManager.addParameter(&custom_analog_min_value_lbl);
  wifiManager.addParameter(&custom_analog_min_value);
  wifiManager.addParameter(&custom_analog_max_value_lbl);
  wifiManager.addParameter(&custom_analog_max_value);
  wifiManager.addParameter(&custom_analog_group_end);
  wifiManager.addParameter(&custom_analog_config_group_end);
  wifiManager.addParameter(&custom_panel_end);

  long wifiTimeStart = millis();

  if (configFileExists){
    //read updated parameters
    IPAddress ip,gateway,mask;
    Serial.println("[Manager] " + network_ip + " " + network_gateway + " " + network_mask + " " + WiFi.macAddress());
    ip.fromString(network_ip);
    gateway.fromString(network_gateway);
    mask.fromString(network_mask);

    String hostname = "Meteo-home_";
    hostname.concat(WiFi.macAddress());
    WiFi.mode(WIFI_STA);
    WiFi.config(ip, gateway,mask);
    Serial.printf("\n[Manager] Configuring network parameters (%s %s %s).\n",ip.toString().c_str(),gateway.toString().c_str(),mask.toString().c_str());  
    WiFi.hostname(hostname.c_str());
    while (WiFi.status() != WL_CONNECTED){
      wifiTimeStart = millis();
      if (strlen(WiFi.psk().c_str())==0){
        WiFi.begin(WiFi.SSID().c_str());
        Serial.printf("\n[Manager] Connecting to an open network (%s).\n",WiFi.SSID().c_str());  
      }
      else {
        WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
        Serial.printf("\n[Manager] Connecting to an encrypted network (%s).\n",WiFi.SSID().c_str());  
      }
      
      while (WiFi.status() != WL_CONNECTED && (millis() - wifiTimeStart < WIFI_CONNECTION_TIMEOUT)) {
        delay(500);
        Serial.print(".");
      }
      
      Serial.println("\n[Manager] Unable to connect to the WiFi network. Trying again.");   
    }
    if (WiFi.status() != WL_CONNECTED){
      Serial.println("\n[Manager] It was unable to connect to the WiFi network. Going to sleep");
      ESP.deepSleep(60  * 1000000);      
    }
    
    Serial.println("\n[Manager] WiFi connected.");
    Serial.println("[Manager] Network configuration:" + WiFi.localIP().toString() + " " + WiFi.gatewayIP().toString() + " " + WiFi.subnetMask().toString() + " " + WiFi.hostname());
  }else {
        shouldSaveConfig=true;
        wifiManager.setTimeout(300);
        //wifiManager.setSaveConnect(false);
        wifiManager.startConfigPortal("Meteo-home");
  }
     
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("[Manager] Saving configuration");
    DynamicJsonDocument json(1024);

    json["network_ip"] = custom_network_ip.getValue();
    json["network_mask"] = custom_network_mask.getValue();
    json["network_gateway"] = custom_network_gateway.getValue();
    
    json["mqtt_server"] = custom_mqtt_server.getValue();
    json["mqtt_port"] = custom_mqtt_port.getValue();
    json["mqtt_user"] = custom_mqtt_username.getValue();
    json["mqtt_password"] = custom_mqtt_password.getValue();
    
    json["use_sleep_mode"] = custom_use_sleep_mode.getValue();
    json["sleep_minutes"] = custom_sleep_minutes.getValue();
    
    json["device_name"] = custom_device_name.getValue();

    json["use_analog_sensor"] = custom_use_analog_sensor.getValue();
    json["sensor_class"] = custom_sensor_class.getValue();
    json["use_arduino_map_function"] = custom_use_map_function.getValue();
    json["use_analog_sensor"] = custom_use_analog_sensor.getValue();
    json["analog_min_value"] = custom_analog_min_value.getValue();
    json["analog_max_value"] = custom_analog_max_value.getValue();
    
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("[Manager] Failed to open config file for writing");
    }

    serializeJson(json, Serial);    
    serializeJson(json, configFile);
    configFile.close();

    Serial.println("[Manager] \nRestarting...");
    ESP.restart();
    //end save
  }
 
}
