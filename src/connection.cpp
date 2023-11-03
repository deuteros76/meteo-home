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

#include "connection.hpp" 

Connection::Connection(Client &client, String addr, String p){
    address.fromString(addr);
    port = atoi(p.c_str());

    setClient(client);
    setServer(address,port);
}

bool Connection::connect(){
  bool returnValue = false;
  const int timeout = 20000;

  // Loop until we're reconnected
  long t1 = millis();

  while (!connected() && (millis() - t1 < timeout)) {
    // Attempt to connect
    String clientName("ESPClient-");
    clientName.concat(ESP.getChipId());
    Serial.print("[Connection] Attempting MQTT connection... ");
    Serial.println(clientName.c_str());
    if (PubSubClient::connect(clientName.c_str())) {
      Serial.println("[Connection] Connected to mqtt");
      returnValue = true;
    } else {
      Serial.print("[Connection] MQTT connection failed, rc=" + String(state()) + " trying again in 5 seconds");
    }
  }
  return returnValue;
}

