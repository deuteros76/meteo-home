#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <PubSubClient.h>
#include "DHT.h"

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"      

#include <ArduinoJson.h> 
#include <FS.h> 
//Temperature sensor settings
#define DHTPIN 2 // what digital pin we're connected to
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

//MQTT  server
char mqtt_server[15] = "192.168.1.5";
char mqtt_port[6] = "8080";
char mqtt_user[30] = "your_username";
char mqtt_password[30] = "YOUR_BLYNK_TOKEN";

//MQTT subscriptions
char dht_temperature_topic[40] = "room/temperature";
char dht_humidity_topic[40] = "room/humidity";
char dht_heatindex_topic[40] = "room/heatindex";
char bmp_pressure_topic[40] = "room/pressure";
char bmp_temperature_topic[40] = "room/device/temperature";


//Deep sleep
#define DEEP_SLEEP_TIME 5 //time in seconds

//Timeout connection wifi or mqtt server
#define CONNECTION_TIMEOUT 200000 //Timeout for connections. The idea is to prevent for continuous/infinit conection tries if the remote device is done. This would cause battery drain

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);
float temperature = 0;
float device_temperature = 0;
float humidity = 0;
float heatindex = 0;
float pressure = 0;

bool configFileExists = false;

Adafruit_BMP085 bmp; // Bmp sensor variable

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  setupFS();
  setup_wifi();
  client.setServer(mqtt_server, 1883); 

  //DHT sensor for humidity and temperature
  dht.begin();
  //BMP180 sensor fro pressure
  bmp.begin();
}

void setup_wifi(){
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

  //reset settings - for testing
  wifiManager.resetSettings();

  int portalTimeout;
  if (configFileExists){
    portalTimeout=20;
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

  if(!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");

    ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);
  } 
 
}

void setupFS(){
  
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        configFileExists = true;
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          //strcpy(mqtt_server, json["mqtt_server"]);
          //strcpy(mqtt_port, json["mqtt_port"]);
          //strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }else{
          Serial.println("failed to open json config file");      
    }
  } else {
    Serial.println("failed to mount FS");
}
}

void reconnect() {
  // Loop until we're reconnected
  long t1 = millis();
  while (!client.connected() && (millis() - t1 < CONNECTION_TIMEOUT)) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      Serial.print("Elapsed time for MQTT:");
      Serial.println((millis()-t1)/1000);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

float ReadTemperatureAndHumidityValue(){
  
  //read dht22 value
  temperature = dht.readTemperature();    
  humidity = dht.readHumidity();
  heatindex = dht.computeHeatIndex(temperature, humidity, false);
     
  Serial.print(temperature);
  Serial.print(" ");
  Serial.print(humidity);
  Serial.print(" ");
  Serial.println(heatindex);
}

/** 
 *  Reads the pressure unsing the BMP180 but also the temperature which is used as the interenal device temperature reference
 */
int readPressure(){  
  //bmp180
  device_temperature = bmp.readTemperature(); 
  pressure = bmp.readPressure()/100.0; //Dividing by 100 we get The pressure in Bars
  Serial.print("Pressure is ");
  Serial.println(pressure);
  Serial.print("Internal temp is ");
  Serial.println(device_temperature);
}

void loop() {
  Serial.print("Starting...");
  long total_time = millis();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
/*
  ReadTemperatureAndHumidityValue();
  client.publish(dht_temperature_topic, String(temperature).c_str(), true);
  client.publish(dht_humidity_topic, String(humidity).c_str(), true);
  client.publish(dht_heatindex_topic, String(heatindex).c_str(), true);
  readPressure();
  client.publish(bmp_pressure_topic, String(pressure).c_str(), true); 
  client.publish(bmp_temperature_topic, String(device_temperature).c_str(), true); 
 */ 
  Serial.print("Elapsed time for this iterration:");
  Serial.println((millis()-total_time)/1000);
  Serial.println("Going to sleep");
  ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);
  //delay(5000);              // wait for a second
}

