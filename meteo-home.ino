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

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "manager.h"

//Temperature sensor settings
#define DHTPIN 12 // what digital pin we're connected to
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

// LEDs pins
#define GREEN_PIN 14
#define YELLOW_PIN 13
#define RED_PIN 15

//Timeout connection for wifi or mqtt server
#define CONNECTION_TIMEOUT 20000 //Timeout for connections. The idea is to prevent for continuous conection tries. This would cause battery drain

bool bmpSensorReady;
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

bool useSleepMode=false;

void setup() {
  //Serial port speed
  t_elapsed = millis();
  Serial.begin(9600);

  // Configuration of LED pins
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);

  digitalWrite(RED_PIN, HIGH);
  digitalWrite(YELLOW_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);

  delay(1000); // Just one second to show that the process has started
  
  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);

  // WiFi setup
  manager.setup_config_data();
  manager.setup_wifi();
  //DHT sensor for humidity and temperature
  dht.begin();
  //BMP180 sensor for pressure
  bmpSensorReady=bmp.begin();
  //Setup mqtt
  IPAddress addr;
  addr.fromString(manager.mqttServer());
  client.setServer(addr, atoi(manager.mqttPort().c_str())); 

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  char buf[256];
  String message = manager.getDiscoveryMsg(manager.dhtTemperatureTopic(), "ºC");
  message.toCharArray(buf, message.length() + 1);
  if (client.beginPublish (manager.dhtTemperatureDiscoveryTopic().c_str(), message.length(), true)) {
    for (int i = 0; i <= message.length() + 1; i++) {
      client.write(buf[i]);
    }
    client.endPublish();
  } else {
    Serial.println("Error sending humidity discovery message");
  }

  // Not sure why but I have to disconnect and connect again to make work the second publish
  client.disconnect();
  reconnect();
  message = manager.getDiscoveryMsg(manager.dhtHumidityTopic(), "%");
  message.toCharArray(buf, message.length() + 1);
  if (client.beginPublish (manager.dhtHumidityDiscoveryTopic().c_str(), message.length(), true)) {
    for (int i = 0; i <= message.length() + 1; i++) {
      client.write(buf[i]);
    }
    client.endPublish();
  } else {
    Serial.println("Error sending humidity discovery message");
  }

  manager.useSleepMode().toLowerCase();
  if (manager.useSleepMode().equals("true")){
    useSleepMode = true;
  } 

  if (!bmpSensorReady){
    digitalWrite(RED_PIN, HIGH);    
  }
  
  digitalWrite(GREEN_PIN, HIGH);  
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
      Serial.print(manager.mqttServer());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void readDHT22(){  
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
void readBMP180(){  
  //bmp180
  if (bmpSensorReady){
    device_temperature = bmp.readTemperature(); 
    pressure = bmp.readPressure()/100.0; //Dividing by 100 we get The pressure in Bars
    Serial.print("Pressure is ");
    Serial.println(pressure);
    Serial.print("Internal temp is ");
    Serial.println(device_temperature);
  }
}

void loop() {  
  Serial.print("Starting...");
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  readDHT22();
  if (isnan(temperature) || isnan(humidity)){
    Serial.print("Error reading DHT22 values");    
  }else {
    client.publish(manager.dhtTemperatureTopic().c_str(), String(temperature).c_str(), true);
    Serial.print(String(manager.dhtTemperatureTopic().c_str()));
    Serial.println(String(temperature).c_str());
    delay(50);
    client.publish(manager.dhtHumidityTopic().c_str(), String(humidity).c_str(), true);
    Serial.print(String(manager.dhtHumidityTopic().c_str()));
    Serial.println(String(humidity).c_str());
    delay(50);
    client.publish(manager.dhtHeatindexTopic().c_str(), String(heatindex).c_str(), true);
    Serial.print(String(manager.dhtHeatindexTopic().c_str()));
    Serial.println(String(heatindex).c_str());
    delay(50);
  }
  readBMP180();
  if (isnan(pressure) || isnan(device_temperature)){
    Serial.print("Error reading DHT22 values");    
  }else {
    client.publish(manager.bmpPressureTopic().c_str(), String(pressure).c_str(), true); 
    Serial.print(String(manager.bmpPressureTopic().c_str()));
    Serial.println(String(pressure).c_str());
    delay(50);
    client.publish(manager.bmpTemperatureTopic().c_str(), String(device_temperature).c_str(), true); 
    Serial.print(String(manager.bmpTemperatureTopic().c_str()));
    Serial.println(String(device_temperature).c_str());
    delay(50);
  }

  if (useSleepMode){
    Serial.print("Going to sleep after ");
    Serial.println((millis()-t_elapsed)/1000);
    ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);
  }else{
    delay(DEEP_SLEEP_TIME * 1000);
  }
}
