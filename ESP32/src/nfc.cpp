#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

// Specific card's UID
byte authorizedUID[4] = {0xCC, 0xA6, 0x56, 0x49}; 

void init_nfc() {
    SPI.begin();
    rfid.PCD_Init();
}

bool check_nfc_card() {
    // Return immediately if no card is tapped
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        return false;
    }

    bool isAuthorized = true;

    // Verify the UID size matches (Standard MIFARE Classic is 4 bytes)
    if (rfid.uid.size == 4) {
        // Compare the scanned card byte-by-byte against the authorized array
        for (byte i = 0; i < 4; i++) {
            if (rfid.uid.uidByte[i] != authorizedUID[i]) {
                isAuthorized = false;
                break; // Mismatch found, stop checking
            }
        }
    } else {
        isAuthorized = false; // Automatically reject if it's not a 4-byte card
    }

    // Clean up and halt the card communication
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    // Will only return true if all 4 bytes perfectly matched the authorizedUID
    return isAuthorized; 
}