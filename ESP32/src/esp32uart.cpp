#include "esp32uart.h"

/*
 * Summary: Implements the UART communication routines on the ESP32 using Hardware 
 * Serial2 (GPIO 16 for RX2, GPIO 17 for TX2). Manages the transmission of the 
 * "AUTH_OK" flag to the MSP430 following a successful NFC patient tap, and listens 
 * for incoming drop confirmations ("DROP_SUCCESS") from the MSP430.
 * Code Structure: esp32_uart_init(), esp32_send_authorization(), esp32_check_messages()
 * Author(s): [Insert Team Member 1], [Insert Team Member 2]
 */

/* Define Hardware Serial2 pins for MSP430 Bridge */
#define MSP_RX_PIN 16  /* ESP32 RX2 connected to MSP430 TX */
#define MSP_TX_PIN 17  /* ESP32 TX2 connected to MSP430 RX */

/*
 * Role: Initializes Serial2 with a 9600 baud rate and dfined RX/TX pins.
 * Parameters: None
 * Returns: None
 */
void esp32_uart_init(void) {
    Serial2.begin(9600, SERIAL_8N1, MSP_RX_PIN, MSP_TX_PIN);
    Serial.println("ESP32 Serial2 UART Bridge Initialized at 9600 baud.");
}

/*
 * Role: Sends the authorization command over Serial2 to wake the MSP430.
 * Parameters: None
 * Returns: None
 */
void esp32_send_authorization(void) {
    Serial2.println("AUTH_OK");  /* Transmit authorization flag to wake MSP430 */
    Serial.println("Sent: Authorization flag sent to MSP430.");
}

/*
 * Role: Listens for incoming data packets sent from the MSP430.
 * Parameters: None
 * Returns: String - incoming message or empty string if nothing available
 */
String esp32_check_messages(void) {
    if (Serial2.available() > 0) {
        String msp_incoming = Serial2.readStringUntil('\n');
        msp_incoming.trim();
        return msp_incoming;
    }
    return "";
}