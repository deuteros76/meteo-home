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

#include "leds.hpp"

Leds::Leds(uint8_t r, uint8_t g, uint8_t y){
  red = r;
  green = g;
  yellow = y;
}

bool Leds::begin(){  // Configuration of LED pins
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(yellow, OUTPUT);

  return true; //! TODO: check this silly function
}

void Leds::setLED(uint8_t led, uint8_t value){
    digitalWrite(led,value);
}

void Leds::setRed(uint8_t value){
    setLED(red,value);
}

void Leds::setGreen(uint8_t value){
    setLED(green,value);
}

void Leds::setYellow(uint8_t value){
    setLED(yellow,value);
}

void Leds::setLEDs(uint8_t rvalue, uint8_t gvalue, uint8_t yvalue){
    setLED(red,rvalue);
    setLED(green,gvalue);
    setLED(yellow,yvalue);
}