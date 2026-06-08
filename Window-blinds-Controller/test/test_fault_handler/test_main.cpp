#include <unity.h>

#include "FaultHandler.hpp"

void setUp(void) {}
void tearDown(void) {}

static void assertFault(const FaultHandler& faults,
                        FaultSource source,
                        FaultReason reason,
                        esp_err_t espErr){
    FaultRecord fault = {FaultSource::BlindsController, FaultReason::None, ESP_OK};

    TEST_ASSERT_TRUE(faults.getFault(fault));
    TEST_ASSERT_EQUAL(source, fault.source);
    TEST_ASSERT_EQUAL(reason, fault.reason);
    TEST_ASSERT_EQUAL(espErr, fault.espErr);
}

void test_fault_handler_starts_empty(void){
    FaultHandler faults;
    FaultRecord fault = {FaultSource::BlindsController, FaultReason::None, ESP_OK};

    TEST_ASSERT_FALSE(faults.hasFault());
    TEST_ASSERT_FALSE(faults.getFault(fault));
}

void test_fault_handler_records_fault(void){
    FaultHandler faults;

    faults.record(FaultSource::MotorController, FaultReason::RmtRefillFailed, ESP_FAIL);

    assertFault(faults, FaultSource::MotorController, FaultReason::RmtRefillFailed, ESP_FAIL);
}

void test_fault_handler_preserves_first_fault(void){
    FaultHandler faults;

    faults.record(FaultSource::BlindsController, FaultReason::MotorStopFailed, ESP_ERR_INVALID_STATE);
    faults.record(FaultSource::MotorController, FaultReason::MovementCompletionFailed, ESP_FAIL);

    assertFault(faults,
                FaultSource::BlindsController,
                FaultReason::MotorStopFailed,
                ESP_ERR_INVALID_STATE);
}

void test_fault_handler_notifies_only_first_fault(void){
    FaultHandler faults;
    faults.setChangeTask(xTaskGetCurrentTaskHandle());
    while(ulTaskNotifyTake(pdTRUE, 0) > 0){}

    faults.record(FaultSource::BlindsController, FaultReason::MotorStopFailed, ESP_FAIL);
    TEST_ASSERT_EQUAL_UINT32(1, ulTaskNotifyTake(pdTRUE, 0));

    faults.record(FaultSource::MotorController, FaultReason::RmtRefillFailed, ESP_ERR_INVALID_STATE);
    TEST_ASSERT_EQUAL_UINT32(0, ulTaskNotifyTake(pdTRUE, 0));
}

extern "C" void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_fault_handler_starts_empty);
    RUN_TEST(test_fault_handler_records_fault);
    RUN_TEST(test_fault_handler_preserves_first_fault);
    RUN_TEST(test_fault_handler_notifies_only_first_fault);

    UNITY_END();
}
