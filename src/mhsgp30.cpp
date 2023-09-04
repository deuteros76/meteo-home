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

#include "mhsgp30.hpp"

MHSGP30::MHSGP30(): SGP30(){
  co2_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) +"/SGP30-CO2/config";
  voc_discovery_topic = "homeassistant/sensor/ESP-" + String(ESP.getChipId()) + "/SGP30-VOC/config";
}

bool MHSGP30::begin(){
  sensorReady=SGP30::begin();
  initAirQuality();
  readBaseline();
  
  co2_topic = manager.deviceName() + "/SGP30/co2";
  voc_topic = manager.deviceName() + "/SGP30/voc";

  return sensorReady;
}

bool MHSGP30::available(){
  bool returnValue=true;

  if (!sensorReady){
    returnValue=false;  
  }
  return returnValue;
}

void MHSGP30::read(){
    //read dht22 value
  measureAirQuality();

  float CO2 = getCO2();
  client.publish(getCO2Topic().c_str(), String(CO2).c_str(), true); 
  delay(50);
  client.publish(getVOCTopic().c_str(), String(getVOC()).c_str(), true); 
  delay(50);
     
  Serial.println("[SGP30] CO2 = " + String(CO2) + " TVOC = " + String(TVOC));
  
  saveBaseline();
}

void MHSGP30::read(float temperature, float humidity){
  if ((humidity > 0) && (temperature > 0)) {
    //Convert relative humidity to absolute humidity
    double absHumidity = RHtoAbsolute(humidity, temperature);

    //Convert the double type humidity to a fixed point 8.8bit number
    uint16_t sensHumidity = doubleToFixedPoint(absHumidity);

    //Set the humidity compensation on the SGP30 to the measured value
    //If no humidity sensor attached, sensHumidity should be 0 and sensor will use default
    setHumidity(sensHumidity);
    Serial.print("[SGP30] Absolute Humidity Compensation set to: " + String(absHumidity) +" g/m^3 ");
  }
  read();
}

String MHSGP30::getDiscoveryMsg(String deviceName, deviceClass dev_class){
  String topic, unit, className;

  switch (dev_class){
    case co2_sensor: unit = "ppm"; className="carbon_dioxide"; topic= deviceName+"/SGP30/co2"; break;
    case voc_sensor: unit = "ppb\t"; className="volatile_organic_compounds"; topic= deviceName+"/SGP30/voc"; break;
    default: break;
  }

  return createDiscoveryMsg(topic, className, unit);
}



void MHSGP30::readBaseline(){
  if (SPIFFS.begin()) {
    //SPIFFS.remove("/baseline.json"); // Uncomment to remove the file if the device sends extremly high values of CO2 / TVOC
    if (SPIFFS.exists("/baseline.json")) {
      //file exists, reading and loading
      File blFile = SPIFFS.open("/baseline.json", "r");
      if (blFile) {
        size_t size = blFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        blFile.readBytes(buf.get(), size);
        DynamicJsonDocument json(1024);
        DeserializationError error =deserializeJson(json,buf.get());
        if (!error) {
          Serial.println("\n[SGP30] parsed json");          
          serializeJson(json, Serial);
        

          baselineCO2 = json["blCO2"].as<uint16_t>();;
          baselineTVOC = json["blTVOC"].as<uint16_t>();;          
          setBaseline(baselineCO2, baselineTVOC);
          Serial.println(String("\n[SGP30] Restoring baseline from file (CO2/TVOC):")+baselineCO2+"/"+baselineTVOC);
        }else{
          Serial.println("\n[SGP30] Baseline json is corrpupt.");      
        }
      }
      blFile.close();
    }else{
      Serial.println("\n[SGP30] Baseline file not found.");      
    }
  }
}

void MHSGP30::saveBaseline(){
  DynamicJsonDocument json(1024);

  getBaseline();  

  if (baselineCO2>0 && baselineTVOC>0 ){
    json["blCO2"] = String(baselineCO2);
    json["blTVOC"] = String(baselineTVOC);
    Serial.println(String("\n[SGP30] Saving new baseline values (CO2,TVOC):")+baselineCO2+"/"+baselineTVOC);
      
    File blFile = SPIFFS.open("/baseline.json", "w");
    if (!blFile) {
      Serial.println("[SGP30] failed to open config file for writing");
    }
    
    serializeJson(json, blFile);
    blFile.close();
  }
 }

 double MHSGP30::RHtoAbsolute (float relHumidity, float tempC) {
  double eSat = 6.11 * pow(10.0, (7.5 * tempC / (237.7 + tempC)));
  double vaporPressure = (relHumidity * eSat) / 100; //millibars
  double absHumidity = 1000 * vaporPressure * 100 / ((tempC + 273) * 461.5); //Ideal gas law with unit conversions
  return absHumidity;
}

uint16_t MHSGP30::doubleToFixedPoint( double number) {
  int power = 1 << 8;
  double number2 = number * power;
  uint16_t value = floor(number2 + 0.5);
  return value;
}
