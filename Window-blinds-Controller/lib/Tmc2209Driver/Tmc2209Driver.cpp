#include "Tmc2209Driver.hpp"

static const char* TAG = "TMC2209";

// TMC2209 register addresses
static constexpr uint8_t REG_GCONF = 0x00;
static constexpr uint8_t REG_IFCNT = 0x02;
static constexpr uint8_t REG_IOIN  = 0x06;
static constexpr uint8_t REG_CHOPCONF = 0x6C;
static constexpr uint8_t REG_IHOLD_IRUN = 0x10;

// Default single-chip address if MS1/MS2 addr pins are low
static constexpr uint8_t TMC_ADDR = 0x00;

Tmc2209Driver::Tmc2209Driver(const TMCUARTDriverPins& UARTPins): UARTPins_(UARTPins){}

static uint8_t tmc_crc(uint8_t* data, size_t len)
{
    uint8_t crc = 0;
    // len includes CRC byte at the end; CRC byte must be 0 while calculating
    for (size_t i = 0; i < len - 1; i++) {
        uint8_t currentByte = data[i];
        for (int j = 0; j < 8; j++) {
            if (((crc >> 7) ^ (currentByte & 0x01)) != 0) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc = (crc << 1);
            }
            currentByte >>= 1;
        }
    }
    return crc;
}

void Tmc2209Driver::init(){
    const uart_config_t uartConfig = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    uart_driver_install(UART_NUM_2, 256, 256, 0, nullptr, 0);
    uart_param_config(UART_NUM_2, &uartConfig);
    uart_set_pin(UART_NUM_2, UARTPins_.TX, UARTPins_.RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE );

    uart_flush(UART_NUM_2);
}

bool Tmc2209Driver::writeReg(uint8_t reg, uint32_t value)
{
    uint8_t frame[8];
    frame[0] = 0x05;                  // sync
    frame[1] = TMC_ADDR;              // slave/node address
    frame[2] = reg | 0x80;            // write bit = 1
    frame[3] = (value >> 24) & 0xFF;  // data3
    frame[4] = (value >> 16) & 0xFF;  // data2
    frame[5] = (value >> 8)  & 0xFF;  // data1
    frame[6] = value & 0xFF;          // data0
    frame[7] = 0x00;                  // placeholder CRC
    frame[7] = tmc_crc(frame, sizeof(frame));

    uart_flush_input(UART_NUM_2);
    int written = uart_write_bytes(UART_NUM_2, reinterpret_cast<const char*>(frame), sizeof(frame));
    uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(20));

    return written == sizeof(frame);
}

bool Tmc2209Driver::readReg(uint8_t reg, uint32_t& value)
{
    uint8_t req[4];
    req[0] = 0x05;
    req[1] = TMC_ADDR;
    req[2] = reg & 0x7F;
    req[3] = 0x00;
    req[3] = tmc_crc(req, sizeof(req));

    uart_flush_input(UART_NUM_2);

    int written = uart_write_bytes(UART_NUM_2, reinterpret_cast<const char*>(req), sizeof(req));
    uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(20));
    if (written != sizeof(req)) {
        return false;
    }

    // 1) discard echoed request bytes
    uint8_t echo[4];
    int echoRead = uart_read_bytes(UART_NUM_2, echo, sizeof(echo), pdMS_TO_TICKS(20));
    if (echoRead != 4) {
        ESP_LOGE(TAG, "Didn't get echo, got %d bytes", echoRead);
        return false;
    }

    ESP_LOGI(TAG, "Echo: %02X %02X %02X %02X", echo[0], echo[1], echo[2], echo[3]);

    // 2) now read actual TMC reply
    uint8_t reply[8] = {0};
    int read = uart_read_bytes(UART_NUM_2, reply, sizeof(reply), pdMS_TO_TICKS(50));
    if (read != 8) {
        ESP_LOGE(TAG, "readReg(0x%02X): expected 8 reply bytes, got %d", reg, read);
        return false;
    }

    ESP_LOGI(TAG, "Reply: %02X %02X %02X %02X %02X %02X %02X %02X",
             reply[0], reply[1], reply[2], reply[3],
             reply[4], reply[5], reply[6], reply[7]);

    if (reply[0] != 0x05 || reply[1] != 0xFF || reply[2] != reg) {
        ESP_LOGE(TAG, "Bad reply header: %02X %02X %02X", reply[0], reply[1], reply[2]);
        return false;
    }

    uint8_t crc = reply[7];
    uint8_t calcBuf[8];
    memcpy(calcBuf, reply, 8);
    calcBuf[7] = 0x00;
    uint8_t calc = tmc_crc(calcBuf, sizeof(calcBuf));
    if (crc != calc) {
        ESP_LOGE(TAG, "CRC mismatch: got %02X calc %02X", crc, calc);
        return false;
    }

    value = (static_cast<uint32_t>(reply[3]) << 24) |
            (static_cast<uint32_t>(reply[4]) << 16) |
            (static_cast<uint32_t>(reply[5]) << 8)  |
            (static_cast<uint32_t>(reply[6]));

    return true;
}

bool Tmc2209Driver::uartSelfTest()
{
    uint32_t ifcntBefore = 0;
    uint32_t ifcntAfter  = 0;
    uint32_t ioin        = 0;

    // 1) Read IFCNT before
    if (!readReg(REG_IFCNT, ifcntBefore)) {
        ESP_LOGE(TAG, "UART FAIL: can't read IFCNT");
        return false;
    }

    if (!writeReg(REG_GCONF, AppConfig::motorGConfig)) {
        ESP_LOGE(TAG, "UART FAIL: can't write GCONF");
        return false;
    }

    uint32_t chopconf = 0;
    if (!readReg(REG_CHOPCONF, chopconf)) {
        ESP_LOGE(TAG, "UART FAIL: can't read CHOPCONF");
        return false;
    }
    ESP_LOGI(TAG, "CHOPCONF = 0x%08lX", (unsigned long)chopconf);

    chopconf = 0x10000053;   // known-good base
    chopconf &= ~(0xFu << 24);        // clear MRES
    chopconf |=  (5u  << 24);         // set 1/8

    if (!writeReg(REG_CHOPCONF, chopconf)) {
        ESP_LOGE(TAG, "UART FAIL: can't write CHOPCONF");
        return false;
    }

constexpr uint32_t ihold_irun =
    (16u << 0)  |   // IHOLD
    (31u << 8)  |   // IRUN = full scale
    (8u  << 16);    // IHOLDDELAY

    writeReg(REG_IHOLD_IRUN, ihold_irun);

    // 3) Read IFCNT again, it should increment by 1 on a successful UART write
    // if (!readReg(REG_IFCNT, ifcntAfter)) {
    //     ESP_LOGE(TAG, "UART FAIL: can't read IFCNT after write");
    //     return false;
    // }

    ESP_LOGI(TAG, "IFCNT before=%lu after=%lu", (unsigned long)ifcntBefore, (unsigned long)ifcntAfter);

    if (((ifcntBefore + 1) & 0xFF) != (ifcntAfter & 0xFF)) {
        ESP_LOGE(TAG, "UART FAIL: IFCNT did not increment");
        // return false;
    }

    // 4) Read IOIN and verify VERSION
    if (!readReg(REG_IOIN, ioin)) {
        ESP_LOGE(TAG, "UART FAIL: can't read IOIN");
        return false;
    }

    uint8_t version = (ioin >> 24) & 0xFF;
    ESP_LOGI(TAG, "IOIN = 0x%08lX, VERSION = 0x%02X", (unsigned long)ioin, version);

    if (version != 0x21) {
        ESP_LOGW(TAG, "UART mostly works, but VERSION is unexpected");
    }

    ESP_LOGI(TAG, "TMC2209 UART OK");
    return true;
}