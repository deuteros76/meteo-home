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
DHT dht(DHTPIN, DHTTYPE); //! Initializes the DHT sensor.
Adafruit_BMP085 bmp; //1 Bmp sensor object
Manager manager;  //! Portal and wific connection manager
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
uint32_t first_boot_done=0; //! Used with deepsleep mode. Send the discovery messages only in the first boot and not after sleeping.


void setup() {
  // Check if this is the first boot (Usefull if using deep sleep mode)
  ESP.rtcUserMemoryRead(0,&first_boot_done,sizeof(first_boot_done)); // Read from persistent RAM memory

  //Serial port speed
  t_elapsed = millis();
  Serial.begin(115200);
  Serial.println("Confifuring device...");

  // Configuration of LED pins
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);

  // Turn on the LEDs to show the device is configureing. In sleep mode, we don't want to use the LEDs everytime the device wakes up
  if (first_boot_done != 1){  
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(YELLOW_PIN, HIGH);
    digitalWrite(GREEN_PIN, HIGH);

    delay(1000); // Just one second to show that the process has started
    
    digitalWrite(RED_PIN, LOW);
    digitalWrite(YELLOW_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
  }

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

  if (first_boot_done != 1){
    first_boot_done = 1;
    ESP.rtcUserMemoryWrite(0,&first_boot_done,sizeof(first_boot_done)); // Write to persistent RAM memory
    char buf[256];
    
    Serial.println("Sending Home Assistant discovery messages.");

    String message = manager.getDiscoveryMsg(manager.dhtTemperatureTopic(), temperature_sensor);
    message.toCharArray(buf, message.length() + 1);
    if (client.beginPublish (manager.dhtTemperatureDiscoveryTopic().c_str(), message.length(), true)) {
      for (int i = 0; i <= message.length() + 1; i++) {
        client.write(buf[i]);
      }
      client.endPublish();
    } else {
      Serial.println("Error sending temperature discovery message");
    }

    // Not sure why but I have to disconnect and connect again to make work the second publish
    client.disconnect();
    reconnect();
    message = manager.getDiscoveryMsg(manager.dhtHumidityTopic(), humidity_sensor);
    message.toCharArray(buf, message.length() + 1);
    if (client.beginPublish (manager.dhtHumidityDiscoveryTopic().c_str(), message.length(), true)) {
      for (int i = 0; i <= message.length() + 1; i++) {
        client.write(buf[i]);
      }
      client.endPublish();
    } else {
      Serial.println("Error sending humidity discovery message");
    }

    client.disconnect();
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
    // Attempt to connect
    String clientName("ESPClient-");
    clientName.concat(ESP.getChipId());
    Serial.print("Attempting MQTT connection... ");
    Serial.println(clientName.c_str());
    if (client.connect(clientName.c_str())) {
      Serial.println("Connected to mqtt");
    } else {
      digitalWrite(RED_PIN, LOW);
      digitalWrite(YELLOW_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      Serial.print("failed, rc=");
      Serial.println(client.state());
      Serial.println(manager.mqttServer());
      Serial.println(manager.mqttPort().c_str());
      Serial.println(" trying again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2500);
      digitalWrite(YELLOW_PIN, HIGH);
      delay(2500);
      digitalWrite(YELLOW_PIN, LOW);
    }
  }
  digitalWrite(YELLOW_PIN, LOW);
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
    Serial.println("Error reading DHT22 values");    
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
  if (isnan(pressure) || pressure== 0 || isnan(device_temperature)){
    Serial.println("Error reading BMP180 values");    
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
