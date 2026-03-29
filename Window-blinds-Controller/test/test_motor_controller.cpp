#include <unity.h>
#include "fakes/FakeMotor.hpp"
#include "BlindsController.hpp"

void setUp(void) {}
void tearDown(void) {}

void test_motor_moving_up(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
}

void test_motor_moving_down(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
}

void test_toggle_style_down_up(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_stop_failure_while_moving_down(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    fMotor.setNextResult(ESP_ERR_INVALID_ARG);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(BlindsEvent::STOP));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_toggle_style_up_down(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_motor_failure(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);
    fMotor.setNextResult(ESP_ERR_INVALID_ARG);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
}

void test_blinds_state_fault_up(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);
    blinds.setState(BlindsState::FAULT);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::NONE, fMotor.getLastAction());
}

void test_blinds_state_fault_down(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);
    blinds.setState(BlindsState::FAULT);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::NONE, fMotor.getLastAction());
}

void test_blinds_state_fault_stop(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);
    blinds.setState(BlindsState::FAULT);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STOP));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_normal_stop_while_moving(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STOP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_stop_failure(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    fMotor.setNextResult(ESP_ERR_INVALID_ARG);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(BlindsEvent::STOP));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_same_button_toggle_up(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_same_button_toggle_down(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_invalid_command(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(static_cast<BlindsEvent>(99)));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::NONE, fMotor.getLastAction());
}

void test_toggle_down_stop_failure_while_moving_up(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    fMotor.setNextResult(ESP_ERR_INVALID_ARG);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_toggle_up_stop_failure_while_moving_down(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    fMotor.setNextResult(ESP_ERR_INVALID_ARG);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

extern "C" void app_main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_motor_moving_up);
    RUN_TEST(test_toggle_style_up_down);
    RUN_TEST(test_motor_failure);
    RUN_TEST(test_blinds_state_fault_up);
    RUN_TEST(test_blinds_state_fault_down);
    RUN_TEST(test_blinds_state_fault_stop);
    RUN_TEST(test_normal_stop_while_moving);
    RUN_TEST(test_stop_failure);
    RUN_TEST(test_same_button_toggle_up);
    RUN_TEST(test_same_button_toggle_down);
    RUN_TEST(test_motor_moving_down);
    RUN_TEST(test_toggle_style_down_up);
    RUN_TEST(test_stop_failure_while_moving_down);
    RUN_TEST(test_invalid_command);
    RUN_TEST(test_toggle_down_stop_failure_while_moving_up);
    RUN_TEST(test_toggle_up_stop_failure_while_moving_down);
    UNITY_END();
}