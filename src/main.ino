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
#include <PubSubClient.h>
#include "mhdht.hpp"
#include "mhbmp.hpp"
#include "mhsgp30.hpp"
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

MHDHT dht(DHTPIN, DHTTYPE); //! Initializes the DHT sensor.
MHBMP bmp; //1 Bmp sensor object
MHSGP30 sgp30; //! Air quality sensor
Manager manager;  //! Portal and wific connection manager
//MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

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
  bmp.begin();
  //SGP30 sensor for air quality
  sgp30.begin();
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

    sendDiscoveryMessage(dht.getTemperatureDiscoveryTopic(), dht.getDiscoveryMsg(manager.deviceName(),MeteoSensor::deviceClass::temperature_sensor));
    sendDiscoveryMessage(dht.getHumidityDiscoveryTopic(), dht.getDiscoveryMsg(manager.deviceName(), MeteoSensor::deviceClass::humidity_sensor));

    if (bmp.available()){
      sendDiscoveryMessage(bmp.getTemperatureDiscoveryTopic(), bmp.getDiscoveryMsg(manager.deviceName(),MeteoSensor::deviceClass::temperature_sensor));
      sendDiscoveryMessage(bmp.getPressureDiscoveryTopic(), bmp.getDiscoveryMsg(manager.deviceName(), MeteoSensor::deviceClass::pressure_sensor));
    }

    if (sgp30.available()){
      sendDiscoveryMessage(sgp30.getCO2DiscoveryTopic(), sgp30.getDiscoveryMsg(manager.deviceName(),MeteoSensor::deviceClass::co2_sensor));
      sendDiscoveryMessage(sgp30.getVOCDiscoveryTopic(), sgp30.getDiscoveryMsg(manager.deviceName(), MeteoSensor::deviceClass::voc_sensor));
    }
  }

  manager.useSleepMode().toLowerCase();
  if (manager.useSleepMode().equals("true")){
    useSleepMode = true;
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

//TODO: discovery message as a parameter
void sendDiscoveryMessage(String discoveryTopic, String message){
    char buf[256];
    reconnect();
    //String message = manager.getDiscoveryMsg(topic, sensorType);
    message.toCharArray(buf, message.length() + 1);
    if (client.beginPublish (discoveryTopic.c_str(), message.length(), true)) {
      for (int i = 0; i <= message.length() + 1; i++) {
        client.write(buf[i]);
      }
      client.endPublish();
    } else {
      Serial.println(String("Error sending discovery message to ") + discoveryTopic);
    }

    client.disconnect();
}

void loop() {  
  Serial.println("Starting main loop...");
  if (!client.connected()) {
    reconnect();
  }  
  client.loop();

  dht.read();
  if (dht.available()){
    client.publish(manager.dhtTemperatureTopic().c_str(), String(dht.getTemperature()).c_str(), true);
    delay(50);
    client.publish(manager.dhtHumidityTopic().c_str(), String(dht.getHumidity()).c_str(), true);
    delay(50);
    client.publish(manager.dhtHeatindexTopic().c_str(), String(dht.getHeatIndex()).c_str(), true);
    delay(50);
  }else{
    Serial.println("Error reading DHT22 values");    
  }
  bmp.read();
  if (bmp.available()){
    client.publish(manager.bmpPressureTopic().c_str(), String(bmp.getPressure()).c_str(), true); 
    delay(50);
    client.publish(manager.bmpTemperatureTopic().c_str(), String(bmp.getTemperature()).c_str(), true); 
    delay(50);
  }else {
    Serial.println("Error reading BMP180 values");    
  }
  if (sgp30.available()){
    if (dht.available()){
      sgp30.read(dht.getTemperature(),dht.getHumidity());
    }else{
      sgp30.read();
    }
    float CO2 = sgp30.getCO2();
    client.publish(manager.sgpCO2Topic().c_str(), String(CO2).c_str(), true); 
    delay(50);
    client.publish(manager.sgpVOCTopic().c_str(), String(sgp30.getVOC()).c_str(), true); 
    delay(50);

    if (CO2 < 600) {
        digitalWrite(YELLOW_PIN, LOW);
        digitalWrite(RED_PIN, LOW);
        digitalWrite(GREEN_PIN, HIGH);
      } else if (CO2 < 800) {
        digitalWrite(GREEN_PIN, LOW);
        digitalWrite(RED_PIN, LOW);
        digitalWrite(YELLOW_PIN, HIGH);
      } else {
        digitalWrite(GREEN_PIN, LOW);
        digitalWrite(YELLOW_PIN, LOW);
        digitalWrite(RED_PIN, HIGH);
      }
  }else {
    Serial.println("Error reading SGP30 values");    
  }


  if (useSleepMode){
    Serial.print("Going to sleep after ");
    Serial.println((millis()-t_elapsed)/1000);
    ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);
  }else{
    delay(DEEP_SLEEP_TIME * 1000);
  }
}
