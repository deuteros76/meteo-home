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
#include "connection.hpp"
#include "meteoboard.hpp"
#include "mhdht.hpp"
#include "mhbmp.hpp"
#include "mhsgp30.hpp"
#include "manager.hpp"
#include "leds.hpp"
#include <vector>

//Temperature sensor settings
#define DHTPIN 12 // what digital pin we're connected to
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

//Timeout connection for wifi or mqtt server
#define CONNECTION_TIMEOUT 20000 //Timeout for connections. The idea is to prevent for continuous conection tries. This would cause battery drain

Manager manager;  //! Portal and wific connection manager
MeteoBoard board;
MHDHT dht(DHTPIN, DHTTYPE); //! Initializes the DHT sensor.
MHBMP bmp; //! Bmp sensor object
Leds leds; //! To mange the three LEDs
MHSGP30 sgp30(&leds); //! Air quality sensor. Leds is a dependency for showing the air quality state

std::vector<std::unique_ptr<MeteoSensor>> sensors; // To store sensors addresses

//MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

long t_elapsed;

bool useSleepMode=false;
uint32_t first_boot_done=0; //! Used with deepsleep mode. Send the discovery messages only in the first boot and not after sleeping.

// (https://arduino-esp8266.readthedocs.io/en/latest/libraries.html) Add the following line to the top of your sketch to use getVcc:
ADC_MODE(ADC_VCC); 


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
      leds.setLEDs(LOW,LOW,LOW);
      Serial.println("failed, rc= " + String(client.state()));
      Serial.println(manager.mqttServer());
      Serial.println(manager.mqttPort().c_str());
      Serial.println("trying again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2500);
      leds.setYellow(HIGH);
      delay(2500);
      leds.setYellow( LOW);
    }
  }
  leds.setYellow(LOW);
}

void setup() {
  char buffer[30];
  // Check if this is the first boot (Usefull if using deep sleep mode)
  ESP.rtcUserMemoryRead(0,&first_boot_done,sizeof(first_boot_done)); // Read from persistent RAM memory

  //Serial port speed
  t_elapsed = millis();
  Serial.begin(115200);
  Serial.println("[Main] Confifuring device...");

  leds.begin();

  // Turn on the LEDs to show the device is configureing. In sleep mode, we don't want to use the LEDs everytime the device wakes up
  if (first_boot_done != 1){  
    leds.setLEDs(HIGH,HIGH,HIGH);
    delay(1000); // Just one second to show that the process has started    
    leds.setLEDs(LOW,LOW,LOW);
  }

  // WiFi setup
  manager.setup_config_data();
  manager.setup_wifi();

  Wire.begin();

  //ESP board
  board.begin();
  //DHT sensor for humidity and temperature
  if (dht.begin()){
    sensors.emplace_back(&dht);
  }
  //BMP180 sensor for pressure  
  if (bmp.begin()){
    sensors.emplace_back(&bmp);
  }
  //SGP30 sensor for air quality
  if (sgp30.begin()){
    sensors.emplace_back(&sgp30);
  }

  //Setup mqtt
  IPAddress addr;
  addr.fromString(manager.mqttServer());
  client.setServer(addr, atoi(manager.mqttPort().c_str())); 

  client.loop();

  if (first_boot_done != 1){
    first_boot_done = 1;
    ESP.rtcUserMemoryWrite(0,&first_boot_done,sizeof(first_boot_done)); // Write to persistent RAM memory
    
    Serial.println("[Main] Sending Home Assistant discovery messages."); 
    board.autodiscover();
    
    for (auto &sensor : sensors) {
      if (sensor->available()){
        sensor->autodiscover();
      }else{
        Serial.println("[Main] Error sending discovery message of sensor ");    
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

  for (auto &sensor : sensors) {
    if (sensor->available()){
      sensor->read();
    }else{ 
      Serial.println("[Main] Error reading sensor values");    
    }
  }
  
  if (useSleepMode){
    Serial.print("[Main] Going to sleep after " + String((millis()-t_elapsed)/1000));
    ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);
  }else{
    delay(DEEP_SLEEP_TIME * 1000);
  }
}

#endif