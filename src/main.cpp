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
#include "mhvoltage.hpp"
#include "manager.hpp"
#include "leds.hpp"

//Temperature sensor settings
#define DHTPIN 12 
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

//Timeout connection for wifi or mqtt server
#define CONNECTION_TIMEOUT 20000 //Timeout for connections. The idea is to prevent for continuous conection tries. This would cause battery drain

//MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

Manager manager;  //! Portal and wific connection manager
MeteoBoard board(&manager, &client);
MHDHT dht(&board, &manager,DHTPIN, DHTTYPE); //! Initializes the DHT sensor.
MHBMP bmp(&board, &manager); //! Bmp sensor object
Leds leds; //! To mange the three LEDs
MHSGP30 sgp30(&board, &manager,&leds); //! Air quality sensor. Leds is a dependency for showing the air quality state
MHVoltage voltage(&board, &manager);

long t_elapsed;

bool useSleepMode=false;
uint32_t first_boot_done=0; //! Used with deepsleep mode. Send the discovery messages only in the first boot and not after sleeping.

ADC_MODE(ADC_VCC); // (https://arduino-esp8266.readthedocs.io/en/latest/libraries.html) Add the following line to the top of your sketch to use getVcc:

void setup() {
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
  //Builtin voltage sensor for humidity and temperature
  if (voltage.begin()){
    board.addSensor(&voltage);
  }
  //DHT sensor for humidity and temperature
  if (dht.begin()){
    board.addSensor(&dht);
  }
  //BMP180 sensor for pressure  
  if (bmp.begin()){
    board.addSensor(&bmp);
  }
  //SGP30 sensor for air quality
  if (sgp30.begin()){
    board.addSensor(&sgp30);
  }

  //Setup mqtt
  //IPAddress addr;
  //addr.fromString(manager.mqttServer());
  //client.setServer(addr, atoi(manager.mqttPort().c_str())); 

  if (first_boot_done != 1){
    first_boot_done = 1;
    ESP.rtcUserMemoryWrite(0,&first_boot_done,sizeof(first_boot_done)); // Write to persistent RAM memory
    
    board.autodiscover(); 
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

  board.processSensors();
  
  if (useSleepMode){
    Serial.print("[Main] Going to sleep after " + String((millis()-t_elapsed)/1000));
    ESP.deepSleep(DEEP_SLEEP_TIME * 1000000);
  }else{
    delay(DEEP_SLEEP_TIME * 1000);
  }
}

#endif