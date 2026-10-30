#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
    Serial.begin(115200);
    SPI.begin();
    rfid.PCD_Init();
}

void loop() {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}