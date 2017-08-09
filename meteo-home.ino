#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "manager.h"

//Temperature sensor settings
#define DHTPIN 12 // what digital pin we're connected to
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

//Timeout connection for wifi or mqtt server
#define CONNECTION_TIMEOUT 20000 //Timeout for connections. The idea is to prevent for continuous conection tries. This would cause battery drain

DHT dht(DHTPIN, DHTTYPE); // Initializes the DHT sensor.
Adafruit_BMP085 bmp; // Bmp sensor object
Manager manager;  //Portal and wific connection manager
//MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

//Global variables for sensor data
float temperature = 0;
float device_temperature = 0;
float humidity = 0;
float heatindex = 0;
float pressure = 0;

long t_elapsed;


void setup() {
  //Serial port speed
  t_elapsed = millis();
  Serial.begin(115200);
  //Prevention of deep sleep failures
  //pinMode(0, INPUT_PULLUP);
  //pinMode(2, INPUT_PULLUP);
  //pinMode(15, LOW);
  //Read setting and launch configuration portal if required
  manager.setup_config_data();
  manager.setup_wifi();
  //DHT sensor for humidity and temperature
  dht.begin();
  //BMP180 sensor fro pressure
  bmp.begin();
  //Setup mqtt
  IPAddress addr;
  addr.fromString(manager.mqttServer());
  client.setServer(addr, atoi(manager.mqttPort().c_str())); 
  Serial.println("Configured!!");
}


void reconnect() {
  // Loop until we're reconnected
  long t1 = millis();
  while (!client.connected() && (millis() - t1 < CONNECTION_TIMEOUT)) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("Connected to mqtt");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

float readDHT22(){  
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
int readBMP180(){  
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
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  readDHT22();
  client.publish(manager.dhtTemperatureTopic().c_str(), String(temperature).c_str(), true);
  client.publish(manager.dhtHumidityTopic().c_str(), String(humidity).c_str(), true);
  client.publish(manager.dhtHeatindexTopic().c_str(), String(heatindex).c_str(), true);
  readBMP180();
  client.publish(manager.bmpPressureTopic().c_str(), String(pressure).c_str(), true); 
  client.publish(manager.bmpTemperatureTopic().c_str(), String(device_temperature).c_str(), true); 

  Serial.print("Going to sleep after ");
  Serial.println((millis()-t_elapsed)/1000);
  ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);
}

