---
layout: page
title: Configuring a meteo-home device
---
After uploading the sketch and the first execution the device will run as an Access Point. Check the available networks for you computer and connect with "meteo-home". After that, point your browser to http://192.168.4.1 and the captive portal will be displayed. 

![Home](pics/home.png) 

Choose the first option ("Configure WiFi") and select your home WiFi network from the list of detected APs and introduce its password. 

![Available networks](pics/wifi-scan.png) 

Now you have to configure your network settings. Below, the fields related to your MQTT server should be introduced. Then, in the "Device parameters" section, it is required to introduce a word identifying your device (you can use a name or the location where it will be installed). This value will be used to create the topics where the data will be sent through MQTT. 

![Settings](pics/parameter-settings.png)
