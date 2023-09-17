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

#ifndef PIO_UNIT_TESTING

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
//#include "connection.hpp"
#include "meteoboard.hpp"
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

Manager manager;  //! Portal and wific connection manager
MeteoBoard board;
MHDHT dht(DHTPIN, DHTTYPE); //! Initializes the DHT sensor.
MHBMP bmp; //1 Bmp sensor object
MHSGP30 sgp30; //! Air quality sensor

uintptr_t sensors[]={};
int sensorIndex=0;

//MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

long t_elapsed;

bool useSleepMode=false;
uint32_t first_boot_done=0; //! Used with deepsleep mode. Send the discovery messages only in the first boot and not after sleeping.

// (https://arduino-esp8266.readthedocs.io/en/latest/libraries.html) Add the following line to the top of your sketch to use getVcc:
ADC_MODE(ADC_VCC); 

void setup() {
  // Check if this is the first boot (Usefull if using deep sleep mode)
  ESP.rtcUserMemoryRead(0,&first_boot_done,sizeof(first_boot_done)); // Read from persistent RAM memory

  //Serial port speed
  t_elapsed = millis();
  Serial.begin(115200);
  Serial.println("[Main] Confifuring device...");

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

  Wire.begin();

  //ESP board
  board.begin();
  //DHT sensor for humidity and temperature
  if (dht.begin()){
    sensors[sensorIndex]=reinterpret_cast<uintptr_t>(&dht);
    sensorIndex++;
  }
  //BMP180 sensor for pressure
  
  if (bmp.begin()){
    sensors[sensorIndex]=reinterpret_cast<uintptr_t>(&bmp);
    sensorIndex++;
  }
  //SGP30 sensor for air quality
  if (sgp30.begin()){
    sensors[sensorIndex]=reinterpret_cast<uintptr_t>(&sgp30);
    sensorIndex++;
  }
  //Setup mqtt
  IPAddress addr;
  addr.fromString(manager.mqttServer());
  client.setServer(addr, atoi(manager.mqttPort().c_str())); 

  //if (!client.connected()) {
  //  reconnect();
  //}
  client.loop();

  if (first_boot_done != 1){
    first_boot_done = 1;
    ESP.rtcUserMemoryWrite(0,&first_boot_done,sizeof(first_boot_done)); // Write to persistent RAM memory
    char buf[256];
    
    Serial.println("[Main] Sending Home Assistant discovery messages.");
 
    board.autodiscover();

    for (int i =0; i<sensorIndex; i++){
      Serial.println("[Main] Sending discovery message for sensor "+String(i));
      MeteoSensor* sensor = reinterpret_cast <MeteoSensor*> (sensors[i]);
      if (sensor->available()){
        sensor->autodiscover();
      }else{
        Serial.println("[Main] Error sending discovert message of sensor " + String(i));    
      }
    }
     
  }

  manager.useSleepMode().toLowerCase();
  if (manager.useSleepMode().equals("true")){
    useSleepMode = true;
  } 
  
  digitalWrite(GREEN_PIN, HIGH);  
  Serial.println("[Main] Configured!!");
}

void reconnect() {
  // Loop until we're reconnected
  long t1 = millis();

  while (!client.connected() && (millis() - t1 < CONNECTION_TIMEOUT)) {
    // Attempt to connect
    String clientName("ESPClient-");
    clientName.concat(ESP.getChipId());
    Serial.print("[Main] Attempting MQTT connection... ");
    Serial.println(clientName.c_str());
    if (client.connect(clientName.c_str())) {
      Serial.println("[Main] Connected to mqtt");
    } else {
      digitalWrite(RED_PIN, LOW);
      digitalWrite(YELLOW_PIN, LOW);
      digitalWrite(GREEN_PIN, LOW);
      Serial.print("failed, rc=");
      Serial.println(client.state());
      Serial.println(manager.mqttServer());
      Serial.println(manager.mqttPort().c_str());
      Serial.println("trying again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2500);
      digitalWrite(YELLOW_PIN, HIGH);
      delay(2500);
      digitalWrite(YELLOW_PIN, LOW);
    }
  }
  digitalWrite(YELLOW_PIN, LOW);
}


void loop() {  
  Serial.println("[Main] Starting main loop...");
  if (!client.connected()) {
    reconnect();
  }  
  client.loop();

  board.read();
  if (board.available()){
    client.publish(board.getVoltageTopic().c_str(), String(board.getVoltage()).c_str(), true); 
    delay(50);
  }else {
    Serial.println("[Main] Error reading ESP board voltage values");    
  }

  for (int i =0; i<sensorIndex; i++){
    Serial.println("[Main] Reading sensor "+String(i));
    MeteoSensor* sensor = reinterpret_cast <MeteoSensor*> (sensors[i]);

    if (sensor->available()){
      sensor->read();
    }else{
      Serial.println("[Main] Error reading sensor " + String(i) + " values");    
    }
  }

  //TOO: improve this code. Do we need a class for managing LEDs?
  if (sgp30.available()){
    float CO2 = sgp30.getCO2();

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
  }


  if (useSleepMode){
    Serial.print("[Main] Going to sleep after ");
    Serial.println((millis()-t_elapsed)/1000);
    ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);
  }else{
    delay(DEEP_SLEEP_TIME * 1000);
  }
}

#endif