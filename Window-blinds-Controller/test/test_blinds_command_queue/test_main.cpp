#include <unity.h>

#include "AppConfig.hpp"
#include "BlindsCommandQueue.hpp"
#include "FaultHandler.hpp"

void setUp(void) {}
void tearDown(void) {}

void test_command_queue_overflow_records_fault(void){
    FaultHandler faults;
    BlindsCommandQueue queue(faults);

    TEST_ASSERT_EQUAL(ESP_OK, queue.init());
    for(uint8_t i = 0; i < AppConfig::commandsQueueDepth; ++i){
        TEST_ASSERT_EQUAL(pdTRUE, queue.send(BlindsEvent::STOP));
    }

    TEST_ASSERT_EQUAL(pdFALSE, queue.send(BlindsEvent::UP));

    FaultRecord fault = {FaultSource::BlindsController, FaultReason::None, ESP_OK};
    TEST_ASSERT_TRUE(faults.getFault(fault));
    TEST_ASSERT_EQUAL(FaultSource::CommandQueue, fault.source);
    TEST_ASSERT_EQUAL(FaultReason::CommandQueueOverflow, fault.reason);
    TEST_ASSERT_EQUAL(ESP_FAIL, fault.espErr);
}

extern "C" void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_command_queue_overflow_records_fault);

    UNITY_END();
}
