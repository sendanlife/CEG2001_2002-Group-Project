#include "nfc.h"
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  21   // Change to 5 if your SDA/SS wire is connected to GPIO 5
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN);

void init_nfc() {
    SPI.begin(18, 19, 23, SS_PIN);   // ESP32 VSPI: SCK=18, MISO=19, MOSI=23
    rfid.PCD_Init();
    delay(50);
    rfid.PCD_SetAntennaGain(rfid.RxGain_max);

    byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
    Serial.print("MFRC522 Firmware Version: 0x");
    Serial.println(version, HEX);
    if (version == 0x00 || version == 0xFF) {
        Serial.println("ERROR: MFRC522 not detected! Check SPI wiring and 3.3V power.");
    }
}

String get_nfc_card() {
    // Auto-recover if a 3.3V Wi-Fi current spike resets the MFRC522 antenna
    byte txControl = rfid.PCD_ReadRegister(rfid.TxControlReg);
    if ((txControl & 0x03) != 0x03 && txControl != 0x00 && txControl != 0xFF) {
        Serial.println("MFRC522 reset detected -> reinitializing...");
        rfid.PCD_Init();
        rfid.PCD_SetAntennaGain(rfid.RxGain_max);
    }

    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        return "";
    }

    String uidString = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) {
            uidString += "0";
        }
        uidString += String(rfid.uid.uidByte[i], HEX);
    }

    uidString.toUpperCase();

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return uidString;
}