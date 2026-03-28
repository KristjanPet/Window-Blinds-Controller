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

void test_motor_failure(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);
    fMotor.setNextResult(ESP_ERR_INVALID_ARG);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(MoveCommand::UP));
    TEST_ASSERT_NOT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
}

void test_blinds_state_fault_up(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);
    blinds.setState(BlindsState::FAULT);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(MoveCommand::UP));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::NONE, fMotor.getLastAction());
}

void test_blinds_state_fault_down(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);
    blinds.setState(BlindsState::FAULT);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(MoveCommand::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::NONE, fMotor.getLastAction());
}

void test_blinds_state_fault_stop(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);
    blinds.setState(BlindsState::FAULT);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(MoveCommand::STOP));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_normal_stop_while_moving(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(MoveCommand::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(MoveCommand::STOP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_stop_failure(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(MoveCommand::UP));
    fMotor.setNextResult(ESP_ERR_INVALID_ARG);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(MoveCommand::STOP));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_same_button_toggle(void){
    FakeMotor fMotor;
    BlindsController blinds(fMotor);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(MoveCommand::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(MoveCommand::UP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
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
    RUN_TEST(test_same_button_toggle);
    UNITY_END();
}