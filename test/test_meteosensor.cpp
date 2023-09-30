 
#include <Arduino.h>
#include <unity.h>
#include <Wire.h>
#include <mhdht.hpp>
#include <mhsgp30.hpp>
#include <mhbmp.hpp>
#include <vector>

//Temperature sensor settings
#define DHTPIN 12 // what digital pin we're connected to
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

Manager manager;  //! Portal and wific connection manager
//MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

std::vector<std::unique_ptr<MeteoSensor>> sensors; // To store sensors addresses

void setup(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_createDHTDiscoveryMsg() {

    MHDHT sensor(DHTPIN, DHTTYPE);
    String test_message("{\"name\":\"sensor.meteohome-test_topic-DHT22-temperature\",\"stat_cla\":\"measurement\",\"dev_cla\":\"temperature\",\"stat_t\":\"test_topic/DHT22/temperature\",\"unit_of_meas\":\"ºC\",\"frc_upd\":true,\"uniq_id\":\"test_topic/DHT22/temperature\"}");
    TEST_ASSERT_TRUE(test_message.equals(sensor.getDiscoveryMsg("test_topic",MeteoSensor::deviceClass::temperature_sensor)));
}

void test_createBMPDiscoveryMsg() {
    MHBMP sensor;
    String test_message("{\"name\":\"sensor.meteohome-test_topic-BMP180-pressure\",\"stat_cla\":\"measurement\",\"dev_cla\":\"atmospheric_pressure\",\"stat_t\":\"test_topic/BMP180/pressure\",\"unit_of_meas\":\"Pa\",\"frc_upd\":true,\"uniq_id\":\"test_topic/BMP180/pressure\"}");
    TEST_ASSERT_TRUE(test_message.equals(sensor.getDiscoveryMsg("test_topic",MeteoSensor::deviceClass::pressure_sensor)));
}

void test_createSGPDiscoveryMsg() {
    MHSGP30 sensor;
    String test_message("{\"name\":\"sensor.meteohome-test_topic-SGP30-co2\",\"stat_cla\":\"measurement\",\"dev_cla\":\"carbon_dioxide\",\"stat_t\":\"test_topic/SGP30/co2\",\"unit_of_meas\":\"ppm\",\"frc_upd\":true,\"uniq_id\":\"test_topic/SGP30/co2\"}");
    TEST_ASSERT_TRUE(test_message.equals(sensor.getDiscoveryMsg("test_topic",MeteoSensor::deviceClass::co2_sensor)));
}

void test_objectIteration() {    
    MHDHT dht(DHTPIN, DHTTYPE);
    MHBMP bmp;
    MHSGP30 sgp30;
    
    sensors.emplace_back(&dht);
    sensors.emplace_back(&bmp);
    sensors.emplace_back(&sgp30);

    String test_message("{\"name\":\"sensor.meteohome-test_topic-DHT22-temperature\",\"stat_cla\":\"measurement\",\"dev_cla\":\"temperature\",\"stat_t\":\"test_topic/DHT22/temperature\",\"unit_of_meas\":\"ºC\",\"frc_upd\":true,\"uniq_id\":\"test_topic/DHT22/temperature\"}");
    TEST_ASSERT_TRUE(test_message.equals(sensors.at(0)->getDiscoveryMsg("test_topic",MeteoSensor::deviceClass::temperature_sensor)));
 
    test_message="{\"name\":\"sensor.meteohome-test_topic-BMP180-pressure\",\"stat_cla\":\"measurement\",\"dev_cla\":\"atmospheric_pressure\",\"stat_t\":\"test_topic/BMP180/pressure\",\"unit_of_meas\":\"Pa\",\"frc_upd\":true,\"uniq_id\":\"test_topic/BMP180/pressure\"}";
    TEST_ASSERT_TRUE(test_message.equals(sensors.at(1)->getDiscoveryMsg("test_topic",MeteoSensor::deviceClass::pressure_sensor)));

    test_message="{\"name\":\"sensor.meteohome-test_topic-SGP30-co2\",\"stat_cla\":\"measurement\",\"dev_cla\":\"carbon_dioxide\",\"stat_t\":\"test_topic/SGP30/co2\",\"unit_of_meas\":\"ppm\",\"frc_upd\":true,\"uniq_id\":\"test_topic/SGP30/co2\"}";
    TEST_ASSERT_TRUE(test_message.equals(sensors.at(2)->getDiscoveryMsg("test_topic",MeteoSensor::deviceClass::co2_sensor)));

    test_message="{\"name\":\"sensor.meteohome-test_topic\",\"stat_cla\":\"measurement\",\"dev_cla\":\"test\",\"stat_t\":\"test_topic\",\"unit_of_meas\":\"%\",\"frc_upd\":true,\"uniq_id\":\"test_topic\"}";

    for (auto &sensor : sensors) {
        TEST_ASSERT_TRUE(test_message.equals(sensor->createDiscoveryMsg("test_topic","test","%")));
    }
}

//int main( int argc, char **argv) {
void loop(){
    UNITY_BEGIN();

    RUN_TEST(test_createDHTDiscoveryMsg);
    RUN_TEST(test_createBMPDiscoveryMsg);
    RUN_TEST(test_createSGPDiscoveryMsg);
    RUN_TEST(test_objectIteration);

    UNITY_END();
    delay(1000);
}
