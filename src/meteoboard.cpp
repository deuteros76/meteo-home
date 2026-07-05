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

MeteoBoard* MeteoBoard::instance = nullptr;

MeteoBoard::MeteoBoard(Manager *m, PubSubClient *c){
  manager = m;
  instance = this;

  client = c;
  
  //Setup mqtt
  IPAddress addr;
  addr.fromString(manager->mqttServer());
  client->setServer(addr, atoi(manager->mqttPort().c_str()));
  client->setCallback(mqttCallback);
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

  // Per-device availability topic (LWT). Kept in a local so its c_str() stays
  // valid for the duration of the connect() call.
  String availabilityTopic = manager->availabilityTopic();

  // Loop until we're reconnected
  long t1 = millis();

  while (!client->connected() && (millis() - t1 < timeout)) {
    String clientName("ESPClient-");
    clientName.concat(ESP.getChipId());
    Serial.print("[Board] Attempting MQTT connection... ");
    Serial.println(clientName.c_str());
    if (client->connect(clientName.c_str(), availabilityTopic.c_str(), 0, true, "offline")) {
      Serial.println("[Board] Connected to mqtt");
      client->publish(availabilityTopic.c_str(), "online", true);
      // Subscribe to Home Assistant birth topic to resend discovery on HASS restart
      client->subscribe("homeassistant/status");
      client->subscribe(manager->configSetTopic().c_str());

      // Publish current config as retained state, so external clients (e.g. Home
      // Assistant) can read it back before issuing a config/set command. Streamed via
      // beginPublish/print/endPublish (same pattern as sendDiscoveryMessage) because the
      // payload can approach PubSubClient's default 256-byte MQTT_MAX_PACKET_SIZE with a
      // long device_name/sensor_class, and publish() would silently fail past that limit.
      String stateTopic = manager->configStateTopic();
      String statePayload = manager->buildConfigStatePayload();
      if (client->beginPublish(stateTopic.c_str(), statePayload.length(), true)) {
        client->print(statePayload);
        client->endPublish();
      } else {
        Serial.println("[Board] Error publishing config/state");
      }
    } else {
      Serial.println("[Board] Failed to connect to mqtt");
      returnValue = false;
    }
  }
  client->loop();

  if (client->connected()) {
    client->publish(availabilityTopic.c_str(), "online", true);
  }

  return returnValue;
}

void MeteoBoard::mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("[Board] MQTT message received on " + String(topic) + ": " + message);

  if (instance == nullptr) {
    return;
  }

  if (String(topic) == "homeassistant/status" && message == "online") {
    Serial.println("[Board] Home Assistant is online, resending discovery messages");
    instance->autodiscover();
  } else if (String(topic) == instance->manager->configSetTopic() && message.length() > 0) {
    // message.length() == 0 is our own retained-clear publish; ignore it to avoid a loop.
    instance->handleConfigCommand(message);
  }
}

void MeteoBoard::sendDiscoveryMessage(String discoveryTopic, String message){
    connectToMQTT();
    if (client->beginPublish(discoveryTopic.c_str(), message.length(), true)) {
      client->print(message);
      if (!client->endPublish()){
        Serial.println(String("[Board] Error publishing discovery message to ") + discoveryTopic);
      }
    } else {
      Serial.println(String("[Board] Error sending discovery message to ") + discoveryTopic);
    }
}

String MeteoBoard::buildConfigResultPayload(const RemoteConfigOutcome &outcome){
  DynamicJsonDocument doc(512);
  String buffer;

  if (outcome.success) {
    doc["status"] = "applied";
    JsonArray fields = doc.createNestedArray("fields");
    for (const String &field : outcome.appliedFields) {
      fields.add(field);
    }
  } else {
    doc["status"] = "error";
    doc["reason"] = outcome.reason;
  }

  serializeJson(doc, buffer);
  return buffer;
}

void MeteoBoard::handleConfigCommand(String payload){
  // Capture both topics before applyRemoteConfig() runs: it can mutate
  // device_name as a side effect, and both topics are derived from it. If we
  // read them afterward, we'd publish to the new device's topics instead of
  // the ones the command actually arrived on.
  String setTopic = manager->configSetTopic();
  String resultTopic = manager->configResultTopic();

  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, payload);

  RemoteConfigOutcome outcome;
  if (err) {
    outcome.success = false;
    outcome.reason = "invalid_payload";
  } else {
    outcome = manager->applyRemoteConfig(doc);
  }

  String resultPayload = buildConfigResultPayload(outcome);
  connectToMQTT();

  // Clear the retained command first to shrink the window where a reconnect
  // inside connectToMQTT() above could redeliver the still-retained command.
  client->publish(setTopic.c_str(), "", true);
  client->publish(resultTopic.c_str(), resultPayload.c_str());

  if (outcome.success) {
    Serial.println("[Board] Remote config applied, restarting...");
    delay(200); // let the publishes above flush before the restart
    ESP.restart();
  }
}
