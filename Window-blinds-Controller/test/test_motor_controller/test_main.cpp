#include <unity.h>
#include <climits>

#include "AppConfig.hpp"
#include "BlindsCommandQueue.hpp"
#include "FaultHandler.hpp"
#include "MotorController.hpp"

void setUp(void) {}
void tearDown(void) {}

void test_motor_rejects_negative_concrete_target(void){
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    MotorController motor(AppConfig::motorPins, queue, faults);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, motor.move(-1));
}

void test_motor_rejects_target_above_configured_max(void){
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    MotorController motor(AppConfig::motorPins, queue, faults);

    motor.setMaxStep(AppConfig::offsetOfMaxStep);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, motor.move(INT_MAX));
}

extern "C" void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_motor_rejects_negative_concrete_target);
    RUN_TEST(test_motor_rejects_target_above_configured_max);

    UNITY_END();
}
