#include "manager.h"

bool shouldSaveConfig=false;

Manager::Manager(){ 
  mqtt_server = "192.168.1.5";
  mqtt_port = "1883";
  mqtt_user = "your_username";
  mqtt_password = "YOUR_PASSWORD";

  //MQTT subscriptions
  dht_temperature_topic = "room/temperature";
  dht_humidity_topic = "room/humidity";
  dht_heatindex_topic = "room/heatindex";
  bmp_pressure_topic = "room/pressure";
  bmp_temperature_topic = "room/device/temperature";
  
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

          mqtt_server = (const char *)json["mqtt_server"];
          mqtt_port = (const char *)json["mqtt_port"];
          mqtt_user = (const char *)json["mqtt_user"];
          mqtt_password = (const char *)json["mqtt_password"];

          dht_temperature_topic = (const char *)json["dht_temperature_topic"];
          dht_humidity_topic = (const char *)json["dht_humidity_topic"];
          dht_heatindex_topic = (const char *)json["dht_heatindex_topic"];
          bmp_pressure_topic = (const char *)json["bmp_pressure_topic"];
          bmp_temperature_topic=(const char *)json["bmp_temperature_topic"];

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
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server.c_str(), 15);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port.c_str(), 6);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password.c_str(), 30);
  WiFiManagerParameter custom_mqtt_username("username", "user name", mqtt_user.c_str(), 30);
  
  WiFiManagerParameter custom_topics_group("<p>MQTT topics</p>");
  WiFiManagerParameter custom_dht_temperature_topic("temperature","temperature",dht_temperature_topic.c_str(),40);
  WiFiManagerParameter custom_dht_humidity_topic("humidity","humidity",dht_humidity_topic.c_str(),40);
  WiFiManagerParameter custom_dht_heatindex_topic("heatindex","heatindex",dht_heatindex_topic.c_str(),40);
  WiFiManagerParameter custom_bmp_pressure_topic("pressure","pressure",bmp_pressure_topic.c_str(),40);
  WiFiManagerParameter custom_bmp_temperature_topic("temperature2","temperature2",bmp_temperature_topic.c_str(),40);
  
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(&Manager::saveConfigCallback);

  //reset settings - for testing
  //wifiManager.resetSettings();

  if (configFileExists){
   wifiManager.setConnectTimeout(20);
    wifiManager.setTimeout(30);
  }else {
    wifiManager.setTimeout(300);
  }
  
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


  long wifiTimeStart = millis();
  if(!wifiManager.autoConnect("Meteo-home")) {
    Serial.println("failed to connect and hit timeout");
    delay(1000);
    Serial.println("Going to sleep");
    ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);
  } 
  
  //if you get here you have connected to the WiFi
  Serial.print("connected...yeey :)");
  Serial.println((millis()-wifiTimeStart)/1000);
    
  //read updated parameters
  mqtt_server=custom_mqtt_server.getValue();
  mqtt_port= custom_mqtt_port.getValue();
  mqtt_user= custom_mqtt_username.getValue();
  mqtt_password=custom_mqtt_password.getValue();

  
  dht_temperature_topic=custom_dht_temperature_topic.getValue();
  dht_humidity_topic=custom_dht_humidity_topic.getValue();
  dht_heatindex_topic=custom_dht_heatindex_topic.getValue();
  bmp_pressure_topic=custom_bmp_pressure_topic.getValue();
  bmp_temperature_topic=custom_bmp_temperature_topic.getValue();

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server.c_str();
    json["mqtt_port"] = mqtt_port.c_str();
    json["mqtt_user"] = mqtt_user.c_str();
    json["mqtt_password"] = mqtt_password.c_str();

    
    json["dht_temperature_topic"] = dht_temperature_topic.c_str();
    json["dht_humidity_topic"] = dht_humidity_topic.c_str();
    json["dht_heatindex_topic"] = dht_heatindex_topic.c_str();
    json["bmp_pressure_topic"] = bmp_pressure_topic.c_str();
    json["bmp_temperature_topic"] = bmp_temperature_topic.c_str();

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
