#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

void init_nfc() {
    SPI.begin();
    rfid.PCD_Init();
    Serial.begin(115200);  
}

// Returns the UID as a hex string like "4396B815", or empty string if no card
String read_nfc_uid() {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        return "";
    }

    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) uid += "0";  // leading zero
        uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return uid;
}

void loop() {
    String uid = read_nfc_uid();
    if (uid.length() > 0) {
        // Prefix makes it easy for the browser to identify NFC lines
        Serial.println(uid);
        delay(300); // debounce
    }
}