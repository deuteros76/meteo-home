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

#include "meteosensor.hpp"

String MeteoSensor::createDiscoveryMsg(String topic,  String dev_class, String unit) {

  DynamicJsonDocument doc(1024);
  String buffer;
  String name = "sensor.meteohome";
  char *token;

  char charBuf[50];
  topic.toCharArray(charBuf, 50);
  
  token = strtok (charBuf,"/");
  while (token != NULL)
  {
    name= name + "-" + token;
    printf ("Token: %s\n",token);
    token = strtok (NULL, "/");
  }
  printf ("Name is: %s\n",name);

  doc["name"] = name;
  doc["stat_cla"] = "measurement";
  doc["dev_cla"] = dev_class;
  doc["stat_t"]   = topic;
  doc["unit_of_meas"] = unit;
  doc["frc_upd"] = true;
  doc["uniq_id"] =  topic;

  serializeJson(doc, buffer);

  return buffer;
}
  