#pragma once
#include <driver/uart.h>
#include "AppConfig.hpp"

class Tmc2209Driver{
private:
    const TMCUARTDriverPins UARTPins_;
    
public:
    Tmc2209Driver(const TMCUARTDriverPins& UARTPins);
    void init();
};
