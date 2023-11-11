 
#include <Arduino.h>
#include <unity.h>
#include <Wire.h>
#include <mhdht.hpp>
#include <mhsgp30.hpp>
#include <mhbmp.hpp>
#include <vector>

#include "tests/test_manager.cpp"
#include "tests/test_meteosensor.cpp"

void setup(void) {
    // set stuff up here
    if (!LittleFS.begin())
    {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    UNITY_BEGIN();

    RUN_TEST(test_createDHTDiscoveryMsg);
    RUN_TEST(test_createBMPDiscoveryMsg);
    RUN_TEST(test_createSGPDiscoveryMsg);
    RUN_TEST(test_objectIteration);

    RUN_TEST(test_fileCreation);
    RUN_TEST(test_setupConfigData);

    UNITY_END();
    delay(1000);
}

void tearDown(void) {
    // clean stuff up here
}

void loop(){
}
