#include "manager.h"
#include <string>

/*
template class std::basic_string<char>;
*/
bool shouldSaveConfig=false;

Manager::Manager(){ 
  
  network_ip = "192.168.1.105";
  network_mask = "255.255.255.0";
  network_gateway = "192.168.1.1";
  
  mqtt_server = "192.168.1.2";
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
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          configFileExists=true;
          Serial.println("\nparsed json");
          
          json.printTo(Serial);

          network_ip = (const char *)json["network_ip"];
          network_mask = (const char *)json["network_mask"];
          network_gateway = (const char *)json["network_gateway"];

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
  
  WiFiManagerParameter custom_topics_group("<p>MQTT topics</p>");
  WiFiManagerParameter custom_dht_temperature_topic("temperature","temperature",dht_temperature_topic.c_str(),40);
  WiFiManagerParameter custom_dht_humidity_topic("humidity","humidity",dht_humidity_topic.c_str(),40);
  WiFiManagerParameter custom_dht_heatindex_topic("heatindex","heatindex",dht_heatindex_topic.c_str(),40);
  WiFiManagerParameter custom_bmp_pressure_topic("pressure","pressure",bmp_pressure_topic.c_str(),40);
  WiFiManagerParameter custom_bmp_temperature_topic("temperature2","temperature2",bmp_temperature_topic.c_str(),40);
  
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
  
  wifiManager.addParameter(&custom_topics_group);
  wifiManager.addParameter(&custom_dht_temperature_topic);
  wifiManager.addParameter(&custom_dht_humidity_topic);
  wifiManager.addParameter(&custom_dht_heatindex_topic);
  wifiManager.addParameter(&custom_bmp_pressure_topic);
  wifiManager.addParameter(&custom_bmp_temperature_topic);


  long wifiTimeStart = millis();

  if (configFileExists){
    //read updated parameters
    network_ip=custom_network_ip.getValue();
    network_mask= custom_network_mask.getValue();
    network_gateway= custom_network_gateway.getValue();
    
    mqtt_server=custom_mqtt_server.getValue();
    mqtt_port= custom_mqtt_port.getValue();
    mqtt_user= custom_mqtt_username.getValue();
    mqtt_password=custom_mqtt_password.getValue();
    
    dht_temperature_topic=custom_dht_temperature_topic.getValue();
    dht_humidity_topic=custom_dht_humidity_topic.getValue();
    dht_heatindex_topic=custom_dht_heatindex_topic.getValue();
    bmp_pressure_topic=custom_bmp_pressure_topic.getValue();
    bmp_temperature_topic=custom_bmp_temperature_topic.getValue();
    
    IPAddress ip,gateway,mask;
    Serial.println(network_ip);
    Serial.println(network_gateway.c_str());
    Serial.println(network_mask.c_str());
    ip.fromString(network_ip.c_str());
    gateway.fromString(network_gateway.c_str());
    mask.fromString(network_mask.c_str());
    
    WiFi.config(ip, gateway,mask);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
    Serial.println(WiFi.SSID().c_str());
    Serial.println( WiFi.psk().c_str());
    Serial.println( WiFi.macAddress());
    
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
  }else {
        wifiManager.setTimeout(300);
        wifiManager.startConfigPortal("Meteo-home");
  }
  
  //if you get here you have connected to the WiFi
  Serial.print("connected...yeey :)");
  Serial.println((millis()-wifiTimeStart)/1000);
    
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();

    json["network_ip"] = custom_network_ip.getValue();
    json["network_mask"] = custom_network_mask.getValue();
    json["network_gateway"] = custom_network_gateway.getValue();
    
    json["mqtt_server"] = custom_mqtt_server.getValue();
    json["mqtt_port"] = custom_mqtt_port.getValue();
    json["mqtt_user"] = custom_mqtt_username.getValue();
    json["mqtt_password"] = custom_mqtt_password.getValue();
    
    json["dht_temperature_topic"] = custom_dht_temperature_topic.getValue();
    Serial.println(custom_dht_temperature_topic.getValue());
    Serial.println("=================");
    json["dht_humidity_topic"] = custom_dht_humidity_topic.getValue();
    json["dht_heatindex_topic"] = custom_dht_heatindex_topic.getValue();
    json["bmp_pressure_topic"] = custom_bmp_pressure_topic.getValue();
    json["bmp_temperature_topic"] = custom_bmp_temperature_topic.getValue();

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
