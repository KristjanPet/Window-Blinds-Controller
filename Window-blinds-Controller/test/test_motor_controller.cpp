#include <unity.h>

#include "fakes/FakeMotor.hpp"
#include "BlindsController.hpp"
#include "BlindsCommandQueue.hpp"

void setUp(void) {}
void tearDown(void) {}

void test_motor_moving_up(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
}

void test_motor_moving_down(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
}

void test_same_button_toggle_up(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_same_button_toggle_down(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_toggle_style_up_down(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_toggle_style_down_up(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_normal_stop_while_moving_up(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STOP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_stop_failure_while_moving_up(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    fMotor.setNextResult(ESP_ERR_INVALID_ARG);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(BlindsEvent::STOP));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_limit_reached_stops_and_sets_idle(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_limit_reached_stop_failure_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    fMotor.setNextResult(ESP_ERR_INVALID_ARG);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_move_up_failure_from_idle(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setNextResult(ESP_ERR_INVALID_ARG);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
}

void test_move_down_failure_from_idle(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setNextResult(ESP_ERR_INVALID_ARG);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
}

void test_calibrate_move_failure_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setNextMoveResult(ESP_ERR_INVALID_STATE);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(BlindsEvent::CALIBRATE));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
}

void test_homing_reached_move_to_max_failure_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    blinds.setState(BlindsState::CALIBRATING_HOME);
    fMotor.setNextMoveResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(BlindsEvent::HOMING_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
}

void test_blinds_state_fault_up(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    blinds.setState(BlindsState::FAULT);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::NONE, fMotor.getLastAction());
}

void test_blinds_state_fault_down(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    blinds.setState(BlindsState::FAULT);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::NONE, fMotor.getLastAction());
}

void test_blinds_state_fault_stop(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    blinds.setState(BlindsState::FAULT);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STOP));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_blinds_state_fault_limit_reached(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    blinds.setState(BlindsState::FAULT);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::NONE, fMotor.getLastAction());
}

extern "C" void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_motor_moving_up);
    RUN_TEST(test_motor_moving_down);
    RUN_TEST(test_same_button_toggle_up);
    RUN_TEST(test_same_button_toggle_down);
    RUN_TEST(test_toggle_style_up_down);
    RUN_TEST(test_toggle_style_down_up);
    RUN_TEST(test_normal_stop_while_moving_up);
    RUN_TEST(test_stop_failure_while_moving_up);
    RUN_TEST(test_limit_reached_stops_and_sets_idle);
    RUN_TEST(test_limit_reached_stop_failure_enters_fault);
    RUN_TEST(test_move_up_failure_from_idle);
    RUN_TEST(test_move_down_failure_from_idle);
    RUN_TEST(test_calibrate_move_failure_enters_fault);
    RUN_TEST(test_homing_reached_move_to_max_failure_enters_fault);
    RUN_TEST(test_blinds_state_fault_up);
    RUN_TEST(test_blinds_state_fault_down);
    RUN_TEST(test_blinds_state_fault_stop);
    RUN_TEST(test_blinds_state_fault_limit_reached);

    UNITY_END();
}
