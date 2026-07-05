 
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

void test_persistConfigRoundTrip() {
    LittleFS.remove("/config.json");

    DynamicJsonDocument json(1024);
    json["network_ip"] = "192.168.1.50";
    json["network_mask"] = "255.255.255.0";
    json["network_gateway"] = "192.168.1.1";
    json["mqtt_server"] = "192.168.1.100";
    json["mqtt_port"] = "1883";
    json["mqtt_user"] = "user";
    json["mqtt_password"] = "pass";
    json["use_sleep_mode"] = "true";
    json["sleep_minutes"] = "15";
    json["device_name"] = "roundtrip_device";
    json["use_analog_sensor"] = "false";
    json["sensor_class"] = "moisture";
    json["use_arduino_map_function"] = "true";
    json["analog_min_value"] = "10";
    json["analog_max_value"] = "900";
    json["config_token"] = "deadbeef";

    File configFile = LittleFS.open("/config.json", "w");
    serializeJson(json, configFile);
    configFile.close();

    Manager manager;
    manager.setup_config_data();

    // Mutate a couple of fields the same way applyRemoteConfig will, then persist.
    manager.setSleepMinutes(30);
    manager.setDeviceName("roundtrip_device_renamed");
    manager.persistConfigForTest();

    Manager reloaded;
    reloaded.setup_config_data();

    TEST_ASSERT_EQUAL_STRING("roundtrip_device_renamed", reloaded.deviceName().c_str());
    TEST_ASSERT_EQUAL(30, reloaded.sleepMinutes());
    TEST_ASSERT_TRUE(reloaded.useSleepMode());
    TEST_ASSERT_FALSE(reloaded.useAnalogSensor());
    TEST_ASSERT_EQUAL_STRING("moisture", reloaded.sensorClass().c_str());
    TEST_ASSERT_EQUAL(10, reloaded.analogMinValue());
    TEST_ASSERT_EQUAL(900, reloaded.analogMaxValue());
    TEST_ASSERT_EQUAL_STRING("deadbeef", reloaded.configToken().c_str());
}

void test_configTokenGeneratedWhenMissing() {
    DynamicJsonDocument json(1024);
    json["network_ip"] = "192.168.1.50";
    json["network_mask"] = "255.255.255.0";
    json["network_gateway"] = "192.168.1.1";
    json["mqtt_server"] = "192.168.1.100";
    json["mqtt_port"] = "1883";
    json["mqtt_user"] = "user";
    json["mqtt_password"] = "pass";
    json["use_sleep_mode"] = "false";
    json["device_name"] = "no_token_device";
    // config_token intencionadamente ausente: simula un config.json de antes de este feature

    File configFile = LittleFS.open("/config.json", "w");
    serializeJson(json, configFile);
    configFile.close();

    Manager manager;
    manager.setup_config_data();

    TEST_ASSERT_EQUAL(32, manager.configToken().length());

    // Debe haberse persistido: al recargar, el mismo token se mantiene (no se regenera cada vez)
    String firstToken = manager.configToken();
    Manager reloaded;
    reloaded.setup_config_data();
    TEST_ASSERT_EQUAL_STRING(firstToken.c_str(), reloaded.configToken().c_str());
}
