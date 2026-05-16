#include <unity.h>
#include <climits>

#include "fakes/FakeMotor.hpp"
#include "AppConfig.hpp"
#include "BlindsController.hpp"
#include "BlindsCommandQueue.hpp"
#include "MotorController.hpp"

void setUp(void) {}
void tearDown(void) {}

static void complete_up_stall_recovery(BlindsController& blinds, FakeMotor& fMotor, int32_t stallStep){
    fMotor.setCurrentStep(stallStep);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::STALL_RECOVERY, blinds.getState());
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
}

void test_motor_moving_up(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
    TEST_ASSERT_EQUAL_UINT32(1, fMotor.getMoveToMaxCalls());
}

void test_motor_moving_down(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(0, fMotor.getLastTargetStep());
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

void test_up_stall_backs_off_and_enters_recovery(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setCurrentStep(10000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::STALL_RECOVERY, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(10000 - AppConfig::normalStallBackoffSteps, fMotor.getLastTargetStep());
    TEST_ASSERT_EQUAL_UINT32(1, fMotor.getStopCalls());
}

void test_down_stall_backs_off_clamped_to_max_and_enters_recovery(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setMaxStepValue(5000);
    fMotor.setCurrentStep(4000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::STALL_RECOVERY, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(5000, fMotor.getLastTargetStep());
}

void test_stall_recovery_limit_reached_retries_original_up_target(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setMaxStepValue(20000);
    fMotor.setCurrentStep(10000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(20000, fMotor.getLastTargetStep());
    TEST_ASSERT_EQUAL_UINT32(1, fMotor.getMoveToMaxCalls());
    TEST_ASSERT_EQUAL_UINT32(2, fMotor.getMoveCalls());
    TEST_ASSERT_EQUAL_UINT32(2, fMotor.getStopCalls());
}

void test_stall_recovery_limit_reached_retries_original_down_target(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setMaxStepValue(20000);
    fMotor.setCurrentStep(10000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(0, fMotor.getLastTargetStep());
    TEST_ASSERT_EQUAL_UINT32(2, fMotor.getStopCalls());
}

void test_stall_recovery_retry_stop_failure_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setCurrentStep(10000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    fMotor.setNextStopResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
    TEST_ASSERT_EQUAL_UINT32(1, fMotor.getMoveToMaxCalls());
}

void test_home_calibration_stall_recovers_and_continues_calibrating_home(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setMaxStepValue(20000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::CALIBRATE));
    fMotor.setCurrentStep(10000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::STALL_RECOVERY, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(10000 + AppConfig::normalStallBackoffSteps, fMotor.getLastTargetStep());

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::CALIBRATING_HOME, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(0, fMotor.getLastTargetStep());
    TEST_ASSERT_TRUE(fMotor.wasLastMoveCalibrating());
}

void test_max_calibration_stall_before_threshold_recovers_and_continues_calibrating_max(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setMaxStepValue(20000);
    fMotor.setCurrentStep(10000);
    blinds.setState(BlindsState::CALIBRATING_MAX);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::STALL_RECOVERY, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(10000 - AppConfig::normalStallBackoffSteps, fMotor.getLastTargetStep());

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::CALIBRATING_MAX, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(20000, fMotor.getLastTargetStep());
    TEST_ASSERT_FALSE(fMotor.wasLastMoveCalibrating());
}

void test_calibration_stall_fourth_recovery_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setMaxStepValue(20000);
    blinds.setState(BlindsState::CALIBRATING_MAX);

    fMotor.setCurrentStep(10000);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));

    fMotor.setCurrentStep(11000);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));

    fMotor.setCurrentStep(12000);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));

    fMotor.setCurrentStep(13000);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_three_stall_recoveries_are_allowed_then_fourth_stall_faults(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));

    complete_up_stall_recovery(blinds, fMotor, 10000);
    complete_up_stall_recovery(blinds, fMotor, 11000);
    complete_up_stall_recovery(blinds, fMotor, 12000);

    fMotor.setCurrentStep(13000);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_target_reached_resets_stall_recovery_count(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));

    complete_up_stall_recovery(blinds, fMotor, 10000);
    complete_up_stall_recovery(blinds, fMotor, 11000);
    complete_up_stall_recovery(blinds, fMotor, 12000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    fMotor.setCurrentStep(13000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::STALL_RECOVERY, blinds.getState());
}

void test_stop_resets_stall_recovery_count(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));

    complete_up_stall_recovery(blinds, fMotor, 10000);
    complete_up_stall_recovery(blinds, fMotor, 11000);
    complete_up_stall_recovery(blinds, fMotor, 12000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STOP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    fMotor.setCurrentStep(13000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::STALL_RECOVERY, blinds.getState());
}

void test_stall_stop_failure_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setCurrentStep(10000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    fMotor.setNextStopResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_stall_recovery_move_failure_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setCurrentStep(10000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    fMotor.setNextMoveResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
}

void test_move_up_failure_from_idle(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    fMotor.setNextResult(ESP_ERR_INVALID_ARG);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
    TEST_ASSERT_EQUAL_UINT32(1, fMotor.getMoveToMaxCalls());
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
    TEST_ASSERT_EQUAL(0, fMotor.getLastTargetStep());
    TEST_ASSERT_TRUE(fMotor.wasLastMoveCalibrating());
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

void test_calibration_returns_to_step_computed_at_homing(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    const int32_t travelFromBoot = 5000;
    const int32_t expectedReturnStep = travelFromBoot + AppConfig::offsetOfMinStep;

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::CALIBRATE));
    fMotor.setCurrentStep(INT_MAX - travelFromBoot);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::HOMING_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::CALIBRATING_MAX, blinds.getState());

    fMotor.setCurrentStep(AppConfig::stepStallThrehold + 10000);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));

    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(expectedReturnStep, fMotor.getLastTargetStep());
}

void test_calibration_return_target_clamps_to_calibrated_max(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    const int32_t travelFromBoot = 40000;
    const int32_t maxStallStep = AppConfig::stepStallThrehold + 10000;
    const int32_t expectedMaxStep = maxStallStep - AppConfig::offsetOfMaxStep;

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::CALIBRATE));
    fMotor.setCurrentStep(INT_MAX - travelFromBoot);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::HOMING_REACHED));

    fMotor.setCurrentStep(maxStallStep);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));

    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(expectedMaxStep, fMotor.getLastTargetStep());
}

void test_calibration_return_move_failure_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::CALIBRATE));
    fMotor.setCurrentStep(INT_MAX - 5000);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::HOMING_REACHED));

    fMotor.setCurrentStep(AppConfig::stepStallThrehold + 10000);
    fMotor.setNextMoveResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
}

void test_up_during_home_calibration_stops_and_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    blinds.setState(BlindsState::CALIBRATING_HOME);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_up_during_max_calibration_stops_and_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    blinds.setState(BlindsState::CALIBRATING_MAX);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_down_during_home_calibration_stops_and_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    blinds.setState(BlindsState::CALIBRATING_HOME);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_down_during_max_calibration_stops_and_enters_fault(void){
    FakeMotor fMotor;
    BlindsCommandQueue queue;
    BlindsController blinds(fMotor, queue);

    blinds.setState(BlindsState::CALIBRATING_MAX);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
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

void test_motor_rejects_negative_concrete_target(void){
    BlindsCommandQueue queue;
    MotorController motor(AppConfig::motorPins, queue);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, motor.move(-1));
}

void test_motor_rejects_target_above_configured_max(void){
    BlindsCommandQueue queue;
    MotorController motor(AppConfig::motorPins, queue);

    motor.setMaxStep(AppConfig::offsetOfMaxStep);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, motor.move(INT_MAX));
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
    RUN_TEST(test_up_stall_backs_off_and_enters_recovery);
    RUN_TEST(test_down_stall_backs_off_clamped_to_max_and_enters_recovery);
    RUN_TEST(test_stall_recovery_limit_reached_retries_original_up_target);
    RUN_TEST(test_stall_recovery_limit_reached_retries_original_down_target);
    RUN_TEST(test_stall_recovery_retry_stop_failure_enters_fault);
    RUN_TEST(test_home_calibration_stall_recovers_and_continues_calibrating_home);
    RUN_TEST(test_max_calibration_stall_before_threshold_recovers_and_continues_calibrating_max);
    RUN_TEST(test_calibration_stall_fourth_recovery_enters_fault);
    RUN_TEST(test_three_stall_recoveries_are_allowed_then_fourth_stall_faults);
    RUN_TEST(test_target_reached_resets_stall_recovery_count);
    RUN_TEST(test_stop_resets_stall_recovery_count);
    RUN_TEST(test_stall_stop_failure_enters_fault);
    RUN_TEST(test_stall_recovery_move_failure_enters_fault);
    RUN_TEST(test_move_up_failure_from_idle);
    RUN_TEST(test_move_down_failure_from_idle);
    RUN_TEST(test_calibrate_move_failure_enters_fault);
    RUN_TEST(test_homing_reached_move_to_max_failure_enters_fault);
    RUN_TEST(test_calibration_returns_to_step_computed_at_homing);
    RUN_TEST(test_calibration_return_target_clamps_to_calibrated_max);
    RUN_TEST(test_calibration_return_move_failure_enters_fault);
    RUN_TEST(test_up_during_home_calibration_stops_and_enters_fault);
    RUN_TEST(test_up_during_max_calibration_stops_and_enters_fault);
    RUN_TEST(test_down_during_home_calibration_stops_and_enters_fault);
    RUN_TEST(test_down_during_max_calibration_stops_and_enters_fault);
    RUN_TEST(test_blinds_state_fault_up);
    RUN_TEST(test_blinds_state_fault_down);
    RUN_TEST(test_blinds_state_fault_stop);
    RUN_TEST(test_blinds_state_fault_limit_reached);
    RUN_TEST(test_motor_rejects_negative_concrete_target);
    RUN_TEST(test_motor_rejects_target_above_configured_max);

    UNITY_END();
}
