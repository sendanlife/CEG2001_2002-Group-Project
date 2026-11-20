#include "nfc.h"
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21 
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

void init_nfc() {
    SPI.begin();
    rfid.PCD_Init();
}

String get_nfc_card() {
    // Return empty string if no card is present
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        return ""; 
    }

    String uidString = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        // Add a leading zero if the hex value is single-digit
        if (rfid.uid.uidByte[i] < 0x10) uidString += "0"; 
        uidString += String(rfid.uid.uidByte[i], HEX);
    }
    
    uidString.toUpperCase(); // Formats to something like "DEADBEEF"

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return uidString; 
}