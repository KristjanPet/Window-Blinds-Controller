#include <unity.h>
#include <cstring>

#include "FaultHandler.hpp"
#include "MqttClient.hpp"

void setUp(void) {}
void tearDown(void) {}

static bool parseMqttPayload(const char* payload, BlindsCommand& command){
    return MqttClient::parseCommandPayload(payload, static_cast<int>(std::strlen(payload)), command);
}

void test_fault_status_payload_without_fault(void){
    FaultHandler faults;
    char payload[160] = {};

    TEST_ASSERT_TRUE(MqttClient::buildFaultStatusPayload(faults, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_STRING("{\"fault\":false}", payload);
}

void test_fault_status_payload_with_fault(void){
    FaultHandler faults;
    char payload[160] = {};
    faults.record(FaultSource::BlindsController, FaultReason::MotorStopFailed, ESP_FAIL);

    TEST_ASSERT_TRUE(MqttClient::buildFaultStatusPayload(faults, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL_STRING("{\"fault\":true,\"source\":\"blinds_controller\",\"reason\":\"motor_stop_failed\",\"esp_err\":-1,\"esp_err_name\":\"ESP_FAIL\"}", payload);
}

void test_fault_status_payload_rejects_small_buffer(void){
    FaultHandler faults;
    char payload[8] = {};

    TEST_ASSERT_FALSE(MqttClient::buildFaultStatusPayload(faults, payload, sizeof(payload)));
}

void test_mqtt_parser_accepts_numeric_percentage(void){
    BlindsCommand command = {BlindsEvent::STOP, 0};

    TEST_ASSERT_TRUE(parseMqttPayload("42", command));
    TEST_ASSERT_EQUAL(BlindsEvent::MOVE_TO_PERCENT, command.event);
    TEST_ASSERT_EQUAL_UINT8(42, command.percent);
}

void test_mqtt_parser_accepts_aliases_and_whitespace(void){
    BlindsCommand command = {BlindsEvent::STOP, 0};

    TEST_ASSERT_TRUE(parseMqttPayload(" up ", command));
    TEST_ASSERT_EQUAL(BlindsEvent::MOVE_TO_PERCENT, command.event);
    TEST_ASSERT_EQUAL_UINT8(100, command.percent);

    TEST_ASSERT_TRUE(parseMqttPayload("DOWN", command));
    TEST_ASSERT_EQUAL(BlindsEvent::MOVE_TO_PERCENT, command.event);
    TEST_ASSERT_EQUAL_UINT8(0, command.percent);

    TEST_ASSERT_TRUE(parseMqttPayload("\tstop\n", command));
    TEST_ASSERT_EQUAL(BlindsEvent::STOP, command.event);
}

void test_mqtt_parser_rejects_invalid_percentage_payloads(void){
    BlindsCommand command = {BlindsEvent::STOP, 0};

    TEST_ASSERT_FALSE(parseMqttPayload("", command));
    TEST_ASSERT_FALSE(parseMqttPayload("101", command));
    TEST_ASSERT_FALSE(parseMqttPayload("-1", command));
    TEST_ASSERT_FALSE(parseMqttPayload("4.2", command));
    TEST_ASSERT_FALSE(parseMqttPayload("42%", command));
}

extern "C" void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_fault_status_payload_without_fault);
    RUN_TEST(test_fault_status_payload_with_fault);
    RUN_TEST(test_fault_status_payload_rejects_small_buffer);
    RUN_TEST(test_mqtt_parser_accepts_numeric_percentage);
    RUN_TEST(test_mqtt_parser_accepts_aliases_and_whitespace);
    RUN_TEST(test_mqtt_parser_rejects_invalid_percentage_payloads);

    UNITY_END();
}
