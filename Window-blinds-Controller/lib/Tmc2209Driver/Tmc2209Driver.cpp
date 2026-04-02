#include "Tmc2209Driver.hpp"

Tmc2209Driver::Tmc2209Driver(const TMCUARTDriverPins& UARTPins): UARTPins_(UARTPins){}

void init(){
    const uart_config_t uartConfig = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

}