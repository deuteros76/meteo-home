#include "manager.h"

bool shouldSaveConfig=false;

Manager::Manager(){ 
  strcpy(mqtt_server, "192.168.1.5");
  strcpy(mqtt_port, "8080");
  strcpy(mqtt_user, "your_username");
  strcpy(mqtt_password, "YOUR_PASSWORD");

  //MQTT subscriptions
  strcpy(dht_temperature_topic, "room/temperature");
  strcpy(dht_humidity_topic, "room/humidity");
  strcpy(dht_heatindex_topic, "room/heatindex");
  strcpy(bmp_pressure_topic, "room/pressure");
  strcpy(bmp_temperature_topic, "room/device/temperature");

  
  //flag for saving data
  shouldSaveConfig = false;
  configFileExists = false;
}

void Manager::setup_config_data(){
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
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
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          configFileExists=true;
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_password, json["mqtt_password"]);

          strcpy(dht_temperature_topic, json["dht_temperature_topic"]);
          strcpy(dht_humidity_topic, json["dht_humidity_topic"]);
          strcpy(dht_heatindex_topic, json["dht_heatindex_topic"]);
          strcpy(bmp_pressure_topic, json["bmp_pressure_topic"]);
          strcpy(bmp_temperature_topic, json["bmp_temperature_topic"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}


void Manager::setup_wifi(){
  WiFiManagerParameter custom_server_group("<p>MQTT Sercer settings</p>");
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 15);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, 30);
  WiFiManagerParameter custom_mqtt_username("username", "user name", mqtt_user, 30);
  
  WiFiManagerParameter custom_topics_group("<p>MQTT topics</p>");
  WiFiManagerParameter custom_dht_temperature_topic("temperature","temperature",dht_temperature_topic,40);
  WiFiManagerParameter custom_dht_humidity_topic("humidity","humidity",dht_humidity_topic,40);
  WiFiManagerParameter custom_dht_heatindex_topic("heatindex","heatindex",dht_heatindex_topic,40);
  WiFiManagerParameter custom_bmp_pressure_topic("pressure","pressure",bmp_pressure_topic,40);
  WiFiManagerParameter custom_bmp_temperature_topic("temperature2","temperature2",bmp_temperature_topic,40);
  
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(&Manager::saveConfigCallback);

  //reset settings - for testing
  //wifiManager.resetSettings();

  int portalTimeout;
  if (configFileExists){
    portalTimeout=300;//20;
  }else {
    portalTimeout=300;    
  }
  
  wifiManager.setTimeout(portalTimeout);
  
  wifiManager.addParameter(&custom_server_group);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  
  wifiManager.addParameter(&custom_topics_group);
  wifiManager.addParameter(&custom_dht_temperature_topic);
  wifiManager.addParameter(&custom_dht_humidity_topic);
  wifiManager.addParameter(&custom_dht_heatindex_topic);
  wifiManager.addParameter(&custom_bmp_pressure_topic);
  wifiManager.addParameter(&custom_bmp_temperature_topic);

  if(!wifiManager.autoConnect("Meteo-home")) {
    Serial.println("failed to connect and hit timeout");
    delay(60000);
    Serial.println("Going to sleep");
    ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);
  } 
  
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
    
  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_username.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());

  
  strcpy(dht_temperature_topic,custom_dht_temperature_topic.getValue());
  strcpy(dht_humidity_topic,custom_dht_humidity_topic.getValue());
  strcpy(dht_heatindex_topic,custom_dht_heatindex_topic.getValue());
  strcpy(bmp_pressure_topic,custom_bmp_pressure_topic.getValue());
  strcpy(bmp_temperature_topic,custom_bmp_temperature_topic.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;

    
    json["dht_temperature_topic"] = dht_temperature_topic;
    json["dht_humidity_topic"] = dht_humidity_topic;
    json["dht_heatindex_topic"] = dht_heatindex_topic;
    json["bmp_pressure_topic"] = bmp_pressure_topic;
    json["bmp_temperature_topic"] = bmp_temperature_topic;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
 
}
