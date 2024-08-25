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

#include "meteoboard.hpp"

MeteoBoard::MeteoBoard(Manager *m, PubSubClient *c){
  manager = m;

  client = c;
  
  //Setup mqtt
  IPAddress addr;
  addr.fromString(manager->mqttServer());
  client->setServer(addr, atoi(manager->mqttPort().c_str())); 
}

bool MeteoBoard::begin(){
  bool returnValue = false;
  if (manager != nullptr){
    returnValue = true;
  }

  return returnValue; //! TODO: think about this boolean function
}

void MeteoBoard::autodiscover(){
  connectToMQTT();

  for (auto &sensor : sensors) {
    Serial.println("[Board] Client state " + String(client->state()));
    connectToMQTT();
    if (sensor->available()){
      sensor->autodiscover();
    }else{
      Serial.println("[Board] Error sending discovery message of sensor ");    
    }
  }
}

void MeteoBoard::processSensors(){
  connectToMQTT();

  for (auto &sensor : sensors) {
    connectToMQTT();
    if (sensor->available()){
      sensor->read();
    }else{ 
      Serial.println("[Board] Error reading sensor values");    
    }
  }  
}

bool MeteoBoard::connectToMQTT(){
  bool returnValue=true;
  const int timeout = 20000;

  // Loop until we're reconnected
  long t1 = millis();

  while (!client->connected() && (millis() - t1 < timeout)) {
    String clientName("ESPClient-");
    clientName.concat(ESP.getChipId());
    Serial.print("[Board] Attempting MQTT connection... ");
    Serial.println(clientName.c_str());
    if (client->connect(clientName.c_str())) {
      Serial.println("[Board] Connected to mqtt");
    } else {
      Serial.println("[Board] Failed to connect to mqtt");
      returnValue = false;
    }
  }
  client->loop();
  
  return returnValue;
}

void MeteoBoard::sendDiscoveryMessage(String discoveryTopic, String message){
    char buf[256];

    connectToMQTT();
    message.toCharArray(buf, message.length() + 1);
    if (client->beginPublish (discoveryTopic.c_str(), message.length(), true)) {
      for (unsigned int i = 0; i <= message.length() + 1; i++) {
        client->write(buf[i]);
      }
      if (!client->endPublish()){
        Serial.println(String("[Board] Error publishing discovery message to ") + discoveryTopic);    
      }
    } else {
      Serial.println(String("[Board] Error sending discovery message to ") + discoveryTopic);
    }
    client->disconnect();
}