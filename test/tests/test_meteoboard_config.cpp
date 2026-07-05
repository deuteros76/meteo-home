#include <Arduino.h>
#include <unity.h>
#include "meteoboard.hpp"

void test_buildConfigResultAppliedPayload() {
    RemoteConfigOutcome outcome;
    outcome.success = true;
    outcome.reason = "applied";
    outcome.appliedFields.push_back("sleep_minutes");
    outcome.appliedFields.push_back("device_name");

    String expected = "{\"status\":\"applied\",\"fields\":[\"sleep_minutes\",\"device_name\"]}";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), MeteoBoard::buildConfigResultPayload(outcome).c_str());
}

void test_buildConfigResultErrorPayload() {
    RemoteConfigOutcome outcome;
    outcome.success = false;
    outcome.reason = "invalid_field:sleep_minutes";

    String expected = "{\"status\":\"error\",\"reason\":\"invalid_field:sleep_minutes\"}";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), MeteoBoard::buildConfigResultPayload(outcome).c_str());
}
