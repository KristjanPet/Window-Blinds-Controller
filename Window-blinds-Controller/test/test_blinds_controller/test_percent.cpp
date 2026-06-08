#include "TestSupport.hpp"

void test_percent_target_moves_up_with_rounding(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    fMotor.setMaxStepValue(101);
    fMotor.setCurrentStep(20);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(percentCommand(50)));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::UP, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(51, fMotor.getLastTargetStep());
}

void test_percent_target_moves_down(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    fMotor.setMaxStepValue(20000);
    fMotor.setCurrentStep(15000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(percentCommand(25)));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(5000, fMotor.getLastTargetStep());
}

void test_percent_endpoint_targets_use_zero_and_max(void){
    {
        FakeMotor fMotor;
        FaultHandler faults;
        BlindsCommandQueue queue(faults);
        BlindsController blinds(fMotor, queue, faults);

        fMotor.setMaxStepValue(12000);
        fMotor.setCurrentStep(6000);

        TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(percentCommand(0)));
        TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
        TEST_ASSERT_EQUAL(0, fMotor.getLastTargetStep());
    }

    {
        FakeMotor fMotor;
        FaultHandler faults;
        BlindsCommandQueue queue(faults);
        BlindsController blinds(fMotor, queue, faults);

        fMotor.setMaxStepValue(12000);
        fMotor.setCurrentStep(6000);

        TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(percentCommand(100)));
        TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
        TEST_ASSERT_EQUAL(12000, fMotor.getLastTargetStep());
    }
}

void test_percent_target_rejected_before_calibrated_max(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(percentCommand(50)));
    TEST_ASSERT_EQUAL(BlindsState::IDLE, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::NONE, fMotor.getLastAction());
}

void test_percent_command_retargets_while_moving(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    fMotor.setMaxStepValue(20000);
    fMotor.setCurrentStep(1000);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(percentCommand(80)));

    fMotor.setCurrentStep(4000);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(percentCommand(10)));

    TEST_ASSERT_EQUAL(BlindsState::MOVING_DOWN, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
    TEST_ASSERT_EQUAL(2000, fMotor.getLastTargetStep());
    TEST_ASSERT_EQUAL_UINT32(1, fMotor.getStopCalls());
    TEST_ASSERT_EQUAL_UINT32(2, fMotor.getMoveCalls());
}

void test_percent_retarget_stop_failure_enters_fault(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    fMotor.setMaxStepValue(20000);
    fMotor.setCurrentStep(1000);
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(percentCommand(80)));
    fMotor.setNextStopResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(percentCommand(10)));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
    assertFault(faults,
                FaultSource::BlindsController,
                FaultReason::MotorStopFailed,
                ESP_FAIL);
}

void test_percent_during_calibration_stops_and_enters_fault(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    blinds.setState(BlindsState::CALIBRATING_HOME);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(percentCommand(50)));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}
