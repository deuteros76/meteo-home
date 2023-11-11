 
#include <Arduino.h>
#include <unity.h>

#include <FS.h>
#include <LittleFS.h>

void test_fileCreation() {

    if (!LittleFS.exists("/test.txt")){
        LittleFS.remove("/test.txt");        
    }
    File file = LittleFS.open("/test.txt", "w");
    file.close();
    TEST_ASSERT_TRUE(LittleFS.exists("/test.txt"));
}

void test_setupConfigData() {

    Manager manager;  

    if (!LittleFS.exists("/config.json")) {
        DynamicJsonDocument json(1024);
        json["network_ip"] = "192.168.1.206";
        json["network_mask"] = "255.255.255.0";
        json["network_gateway"] = "192.168.1.1";
        
        json["mqtt_server"] = "192.168.1.100";
        json["mqtt_port"] = "1883";
        json["mqtt_user"] =  "";
        json["mqtt_password"] = "";
        
        json["use_sleep_mode"] = "False";
        json["device_name"] = "test_device";
        
        File configFile = LittleFS.open("/config.json", "w");
        if (!configFile) {
        Serial.println("[Manager] Failed to open config file for writing");
        }
        serializeJson(json, configFile);
        configFile.close();
    }              
              
    manager.setup_config_data();
    TEST_ASSERT_TRUE(manager.mqttPort().length()>0);                                                                        
}
