/*
Copyright 2023 meteo-home

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
#ifndef _MHLEDS_
#define _MHLEDS_

#include <Arduino.h>

// LEDs pins
#define GREEN_PIN 14
#define YELLOW_PIN 13
#define RED_PIN 15

/**
 * @brief This class manages the three LEDs which are expected to be connected to the board. They are used at boot for showing errors / status and in the loop code they
 * are used by the SGP30 sensor (if connected) to report about the CO2 values * 
 */
class Leds{
  public:
    Leds(uint8_t r = RED_PIN, uint8_t g = GREEN_PIN, uint8_t y = YELLOW_PIN);   
    bool begin(); //! Setup pins

    void setRed(uint8_t value);
    void setGreen(uint8_t value);
    void setYellow(uint8_t value);
    void setLEDs(uint8_t rvalue, uint8_t gvalue, uint8_t yvalue);

  private:
    uint8_t red;
    uint8_t yellow;
    uint8_t green;

    void setLED(uint8_t led, uint8_t value);
};

#endif