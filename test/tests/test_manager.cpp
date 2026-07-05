 
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

Manager makeConfiguredManager() {
    LittleFS.remove("/config.json");
    Manager manager;
    manager.setDeviceName("device_under_test");
    manager.setSleepMinutes(5);
    manager.setUseSleepMode(false);
    manager.setUseAnalogSensor(true);
    manager.setSensorClass("moisture");
    manager.setUseArduinoMapFunction(true);
    manager.setAnalogMinValue(0);
    manager.setAnalogMaxValue(1024);
    manager.persistConfigForTest();
    // El token no se genera hasta setup_config_data(); para test lo forzamos leyendo de disco.
    manager.setup_config_data();
    return manager;
}

void test_applyRemoteConfigValidPartialUpdate() {
    Manager manager = makeConfiguredManager();
    String token = manager.configToken();

    DynamicJsonDocument doc(256);
    doc["token"] = token;
    doc["sleep_minutes"] = 20;
    doc["device_name"] = "renamed_device";

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_TRUE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("applied", outcome.reason.c_str());
    TEST_ASSERT_EQUAL(2, outcome.appliedFields.size());
    TEST_ASSERT_EQUAL(20, manager.sleepMinutes());
    TEST_ASSERT_EQUAL_STRING("renamed_device", manager.deviceName().c_str());
    // Campos no incluidos en el payload no cambian
    TEST_ASSERT_FALSE(manager.useSleepMode());
    TEST_ASSERT_EQUAL_STRING("moisture", manager.sensorClass().c_str());
}

void test_applyRemoteConfigInvalidToken() {
    Manager manager = makeConfiguredManager();

    DynamicJsonDocument doc(256);
    doc["token"] = "wrong-token";
    doc["sleep_minutes"] = 20;

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("invalid_token", outcome.reason.c_str());
    TEST_ASSERT_EQUAL(5, manager.sleepMinutes()); // sin cambios
}

void test_applyRemoteConfigMissingToken() {
    Manager manager = makeConfiguredManager();

    DynamicJsonDocument doc(256);
    doc["sleep_minutes"] = 20;

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("invalid_token", outcome.reason.c_str());
}

void test_applyRemoteConfigInvalidSleepMinutes() {
    Manager manager = makeConfiguredManager();
    String token = manager.configToken();

    DynamicJsonDocument doc(256);
    doc["token"] = token;
    doc["sleep_minutes"] = 61; // fuera de rango 1-60
    doc["device_name"] = "should_not_apply";

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("invalid_field:sleep_minutes", outcome.reason.c_str());
    // Transaccional: ni siquiera device_name se aplica
    TEST_ASSERT_EQUAL_STRING("device_under_test", manager.deviceName().c_str());
}

void test_applyRemoteConfigInvalidAnalogRange() {
    Manager manager = makeConfiguredManager();
    String token = manager.configToken();

    DynamicJsonDocument doc(256);
    doc["token"] = token;
    doc["analog_min_value"] = 500;
    doc["analog_max_value"] = 100; // min >= max, invalido

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("invalid_field:analog_min_value", outcome.reason.c_str());
}

void test_applyRemoteConfigEmptyDeviceName() {
    Manager manager = makeConfiguredManager();
    String token = manager.configToken();

    DynamicJsonDocument doc(256);
    doc["token"] = token;
    doc["device_name"] = "";

    RemoteConfigOutcome outcome = manager.applyRemoteConfig(doc);

    TEST_ASSERT_FALSE(outcome.success);
    TEST_ASSERT_EQUAL_STRING("invalid_field:device_name", outcome.reason.c_str());
}

void test_buildConfigStatePayloadExcludesTokenAndUsesNativeTypes() {
    Manager manager = makeConfiguredManager();

    String payload = manager.buildConfigStatePayload();

    DynamicJsonDocument doc(384);
    DeserializationError err = deserializeJson(doc, payload);
    TEST_ASSERT_FALSE(err);

    TEST_ASSERT_FALSE(doc.containsKey("token"));
    TEST_ASSERT_FALSE(doc.containsKey("config_token"));

    TEST_ASSERT_EQUAL_STRING("device_under_test", doc["device_name"].as<const char*>());
    TEST_ASSERT_TRUE(doc["use_sleep_mode"].is<bool>());
    TEST_ASSERT_FALSE(doc["use_sleep_mode"].as<bool>());
    TEST_ASSERT_TRUE(doc["sleep_minutes"].is<int>());
    TEST_ASSERT_EQUAL(5, doc["sleep_minutes"].as<int>());
    TEST_ASSERT_TRUE(doc["use_analog_sensor"].is<bool>());
    TEST_ASSERT_TRUE(doc["use_analog_sensor"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("moisture", doc["sensor_class"].as<const char*>());
    TEST_ASSERT_TRUE(doc["use_arduino_map_function"].is<bool>());
    TEST_ASSERT_TRUE(doc["use_arduino_map_function"].as<bool>());
    TEST_ASSERT_EQUAL(0, doc["analog_min_value"].as<int>());
    TEST_ASSERT_EQUAL(1024, doc["analog_max_value"].as<int>());

    TEST_ASSERT_EQUAL(8, doc.size());
}
