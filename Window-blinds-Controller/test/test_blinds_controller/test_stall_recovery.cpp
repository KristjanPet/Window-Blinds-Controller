#include "TestSupport.hpp"
#include "AppConfig.hpp"

static void complete_up_stall_recovery(BlindsController& blinds, FakeMotor& fMotor, int32_t stallStep){
    fMotor.setCurrentStep(stallStep);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::STALL_RECOVERY, blinds.getState());
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::MOVING_UP, blinds.getState());
}

void test_up_stall_backs_off_and_enters_recovery(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

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
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

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
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

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
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

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
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    fMotor.setCurrentStep(10000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    fMotor.setNextStopResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(BlindsEvent::LIMIT_REACHED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
    TEST_ASSERT_EQUAL_UINT32(1, fMotor.getMoveToMaxCalls());
}

void test_three_stall_recoveries_are_allowed_then_fourth_stall_faults(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));

    complete_up_stall_recovery(blinds, fMotor, 10000);
    complete_up_stall_recovery(blinds, fMotor, 11000);
    complete_up_stall_recovery(blinds, fMotor, 12000);

    fMotor.setCurrentStep(13000);

    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
    assertFault(faults,
                FaultSource::BlindsController,
                FaultReason::StallRecoveryExhausted,
                ESP_ERR_INVALID_STATE);
}

void test_target_reached_resets_stall_recovery_count(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

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
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

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
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    fMotor.setCurrentStep(10000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    fMotor.setNextStopResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::STOP, fMotor.getLastAction());
}

void test_stall_recovery_move_failure_enters_fault(void){
    FakeMotor fMotor;
    FaultHandler faults;
    BlindsCommandQueue queue(faults);
    BlindsController blinds(fMotor, queue, faults);

    fMotor.setCurrentStep(10000);

    TEST_ASSERT_EQUAL(ESP_OK, blinds.handleCommand(BlindsEvent::UP));
    fMotor.setNextMoveResult(ESP_FAIL);

    TEST_ASSERT_EQUAL(ESP_FAIL, blinds.handleCommand(BlindsEvent::STALL_DETECTED));
    TEST_ASSERT_EQUAL(BlindsState::FAULT, blinds.getState());
    TEST_ASSERT_EQUAL(LastAction::DOWN, fMotor.getLastAction());
}
