#include <unity.h>
#include "fakes/FakeMotor.hpp"
#include "BlindsController.hpp"

void setUp(void) {}
void tearDown(void) {}

void test_motor_moving_up(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(MoveCommand::UP));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
}

void test_toggle_style_up_down(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(MoveCommand::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(MoveCommand::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

extern "C" void app_main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_motor_moving_up);
    RUN_TEST(test_toggle_style_up_down);
    UNITY_END();
}