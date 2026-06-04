#pragma once

#include <unity.h>

#include "../fakes/FakeMotor.hpp"
#include "BlindsCommandQueue.hpp"
#include "BlindsController.hpp"
#include "FaultHandler.hpp"

inline BlindsCommand percentCommand(uint8_t percent){
    return {BlindsEvent::MOVE_TO_PERCENT, percent};
}

inline void assertFault(const FaultHandler& faults,
                        FaultSource source,
                        FaultReason reason,
                        esp_err_t espErr){
    FaultRecord fault = {FaultSource::BlindsController, FaultReason::None, ESP_OK};

    TEST_ASSERT_TRUE(faults.getFault(fault));
    TEST_ASSERT_EQUAL(source, fault.source);
    TEST_ASSERT_EQUAL(reason, fault.reason);
    TEST_ASSERT_EQUAL(espErr, fault.espErr);
}
