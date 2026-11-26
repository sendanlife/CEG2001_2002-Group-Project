#include "esp32uart.h"

#define MSP_RX_PIN 16  
#define MSP_TX_PIN 17  

void esp32_uart_init(void) {
    Serial2.begin(9600, SERIAL_8N1, MSP_RX_PIN, MSP_TX_PIN);
    Serial.println("ESP32 Serial2 UART Bridge Initialized at 9600 baud.");
}

void esp32_send_authorization(void) {
    Serial2.println("AUTH_OK");
    Serial.println("Sent: Authorization flag sent to MSP430.");
}

String esp32_check_messages(void) {
    if (Serial2.available() > 0) {
        String msp_incoming = Serial2.readStringUntil('\n');
        msp_incoming.trim();
        return msp_incoming;
    }
    return "";
}