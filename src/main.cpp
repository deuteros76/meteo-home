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
#include "mhanalog.hpp"
#include "mhbmp.hpp"
#include "mhsgp30.hpp"
#include "mhaht20.hpp"
#include "mhvoltage.hpp"
#include "manager.hpp"
#include "leds.hpp"

//Temperature sensor settings
#define DHTPIN 12 
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

#define RTC_FIRST_USABLE_ADDRESS 65 // Must be >64 as explained here https://github.com/esp8266/Arduino/blob/24c41524dc56c683d8926671bdd639d7411f2815/cores/esp8266/Esp.cpp#L154

//Timeout connection for wifi or mqtt server
#define CONNECTION_TIMEOUT 20000 //Timeout for connections. The idea is to prevent for continuous conection tries. This would cause battery drain

//MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

Manager manager;  //! Portal and wific connection manager
MeteoBoard board(&manager, &client);
MHDHT dht(&board, &manager,DHTPIN, DHTTYPE); //! Initializes the DHT sensor.
MHBMP bmp(&board, &manager); //! Bmp sensor object
MHAHT20 aht20(&board, &manager); //! Bmp sensor object
Leds leds; //! To mange the three LEDs
MHSGP30 sgp30(&board, &manager,&leds); //! Air quality sensor. Leds is a dependency for showing the air quality state
MHVoltage voltage(&board, &manager);
MHAnalog analog(&board, &manager); //! Analog sensor attached to the board

long t_elapsed;

bool useSleepMode=false;
bool useAnalogSensor=true; //! In ESP8266 we cannot use an analog sensor and meassure VCC input at the same time. This variable is used to block the VCC mode.
uint32_t first_boot_done=0; //! Used with deepsleep mode. Send the discovery messages only in the first boot and not after sleeping.

typedef struct {
  bool useAnalogSensor=true;
  uint32_t first_boot_done=0 ;
} rtcStore;

int get_ADC(){
  rtcStore rtcMem; 
  system_rtc_mem_read(RTC_FIRST_USABLE_ADDRESS, &rtcMem, sizeof(rtcMem));
  first_boot_done=rtcMem.first_boot_done;
  useAnalogSensor=rtcMem.useAnalogSensor;
  
  //! When is booting for the first time after pluging the power wire, we assume the memory is filled with 0s. If we press reset the memory will keep its values but
  //! is prefearable to let ADC_TOUT when first_boot_done is 0 
  if (first_boot_done==0){ 
    return ADC_TOUT;
  }
  else if (useAnalogSensor){
    return ADC_TOUT;
  }else{
    return ADC_VCC;
  }
}

ADC_MODE(get_ADC()); // Normally ADC_MODE(ADC_VCC) is used but we want to select between VCC and TOUT (https://arduino-esp8266.readthedocs.io/en/latest/libraries.html) Add the following line to the top of your sketch to use getVcc:


void setup() {
  // Check if this is the first boot (Usefull if using deep sleep mode)
  rtcStore rtcMem; 
  system_rtc_mem_read(RTC_FIRST_USABLE_ADDRESS, &rtcMem, sizeof(rtcMem)); // Read from persistent RAM memory
  first_boot_done=rtcMem.first_boot_done;

  //Serial port speed
  t_elapsed = millis();
  Serial.begin(9600);
  Serial.println("[Main] Confifuring device...");

  // WiFi setup
  manager.setup_config_data();
  manager.setup_wifi();
  
  //! At this poing we need to know if the analog sensor was selected by the user
  useAnalogSensor=manager.useAnalogSensor();

  leds.begin();

  // Turn on the LEDs to show the device is configureing. In sleep mode, we don't want to use the LEDs everytime the device wakes up
  if (first_boot_done != 1){  
    leds.setLEDs(HIGH,HIGH,HIGH);
    delay(1000); // Just one second to show that the process has started    
    leds.setLEDs(LOW,LOW,LOW);

    rtcMem.first_boot_done=1; //! Warnig! We write 1 to rtcmem but we still need the global first_boot_done be 0
    rtcMem.useAnalogSensor=useAnalogSensor;
    system_rtc_mem_write(RTC_FIRST_USABLE_ADDRESS, &rtcMem, sizeof(rtcMem)); // Write to persistent RAM memory
  }

  Wire.begin();

  //ESP board
  board.begin();
  
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
  //AHT20 sensor for humidity and temperature
  if (aht20.begin()){
    board.addSensor(&aht20);
  }

  if (useAnalogSensor){ //! If we use the analog input we will avoid to meassure the VCC input.
    //Analog sensor 
    if (analog.begin()){
      board.addSensor(&analog);
    }
  }
  else{
    //Builtin voltage sensor 
    if (voltage.begin()){
      board.addSensor(&voltage);
    }
  }

  if (first_boot_done != 1){
    first_boot_done = 1;
    
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
    Serial.print("[Main] Going to sleep for " + String(manager.sleepMinutes())+" minutes after " + String((millis()-t_elapsed)/1000.0)+ " seconds");
    ESP.deepSleep(manager.sleepMinutes() * 60 * 1000000);
  }else{
    delay(manager.sleepMinutes() * 60 * 1000);
  }
}

#endif