#include <climits>

#include "TestSupport.hpp"
#include "AppConfig.hpp"

void test_home_calibration_stall_recovers_and_continues_calibrating_home(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

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
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

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
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

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

void test_calibrate_move_failure_enters_fault(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    fMotor.setNextMoveResult(ESP_ERR_INVALID_STATE);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(BlindsEvent::CALIBRATE));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(0, fMotor.getLastTargetStep());
    TEST_ASSERT_TRUE(fMotor.wasLastMoveCalibrating());
    assertFault(faults,
                FaultSource::BlindsController,
                FaultReason::MotorMoveFailed,
                ESP_ERR_INVALID_STATE);
}

void test_homing_reached_move_to_max_failure_enters_fault(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    blinds.setState(BlindsState::CALIBRATING_HOME);
    fMotor.setNextMoveResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(BlindsEvent::HOMING_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
}

void test_calibration_returns_to_step_computed_at_homing(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    const int32_t travelFromBoot = 5000;
    const int32_t expectedReturnStep = travelFromBoot + AppConfig::offsetOfMinStep;

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::CALIBRATE));
    fMotor.setCurrentStep(INT_MAX - travelFromBoot);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::HOMING_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::CALIBRATING_MAX, blinds.getState());

    fMotor.setCurrentStep(AppConfig::stepStallThreshold + 10000);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));

    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(expectedReturnStep, fMotor.getLastTargetStep());
}

void test_calibration_return_target_clamps_to_calibrated_max(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    const int32_t travelFromBoot = 40000;
    const int32_t maxStallStep = AppConfig::stepStallThreshold + 10000;
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
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::CALIBRATE));
    fMotor.setCurrentStep(INT_MAX - 5000);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::HOMING_REACHED));

    fMotor.setCurrentStep(AppConfig::stepStallThreshold + 10000);
    fMotor.setNextMoveResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
}

void test_up_during_home_calibration_stops_and_enters_fault(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    blinds.setState(BlindsState::CALIBRATING_HOME);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_up_during_max_calibration_stops_and_enters_fault(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    blinds.setState(BlindsState::CALIBRATING_MAX);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_down_during_home_calibration_stops_and_enters_fault(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    blinds.setState(BlindsState::CALIBRATING_HOME);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_down_during_max_calibration_stops_and_enters_fault(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    blinds.setState(BlindsState::CALIBRATING_MAX);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::DOWN));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}
