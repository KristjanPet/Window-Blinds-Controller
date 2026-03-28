#include <unity.h>
#include "fakes/FakeMotor.hpp"
#include "BlindsController.hpp"

void setUp(void) {}
void tearDown(void) {}

void test_motor_moving_up(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(MoveCommand::UP));
}

extern "C" void app_main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_motor_moving_up);
    UNITY_END();
}