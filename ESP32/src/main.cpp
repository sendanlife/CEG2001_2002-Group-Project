#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "nfc.h"

#define MSP430_RX_PIN 16
#define MSP430_TX_PIN 17

const char* ssid = "yoe";
const char* password = "pword123";

const String baseUrl                = "http://10.215.148.203:3000";
const String nfcEndpoint            = baseUrl + "/api/auth/verify-nfc";
const String tempEndpoint           = baseUrl + "/api/temperature";
const String dispenseStatusEndpoint = baseUrl + "/api/dispense-status";
const String weightEndpoint         = baseUrl + "/api/weight";
const String pickupStatusEndpoint   = baseUrl + "/api/pickup-status";
const String pendingNfcEndpoint     = baseUrl + "/api/auth/pending-nfc";

String uartBuffer = "";

void postTemperatureToFlask(float tempC) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: Wi-Fi Disconnected (temp skipped)");
        return;
    }

    HTTPClient http;
    http.setTimeout(1500);   // Keep short so slow Wi-Fi never blocks NFC polling
    http.begin(tempEndpoint);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"temperature\":" + String(tempC, 1) + "}";
    int httpCode = http.POST(payload);
    Serial.printf("Temp %.1f C -> HTTP %d\n", tempC, httpCode);
    http.end();
}

void postDispenseStatusToFlask(const String& pillSize, const String& status) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: Wi-Fi Disconnected (dispense status skipped)");
        return;
    }

    HTTPClient http;
    http.setTimeout(4000);
    http.begin(dispenseStatusEndpoint);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"pillSize\":\"" + pillSize + "\",\"status\":\"" + status + "\"}";
    int httpCode = http.POST(payload);
    Serial.printf("Dispense (%s) %s -> HTTP %d\n", pillSize.c_str(), status.c_str(), httpCode);
    http.end();
}

void postWeightToFlask(const String& pillSize, float weightG) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: Wi-Fi Disconnected (weight skipped)");
        return;
    }

    HTTPClient http;
    http.setTimeout(4000);
    http.begin(weightEndpoint);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"pillSize\":\"" + pillSize + "\",\"weight\":" + String(weightG, 2) + "}";
    int httpCode = http.POST(payload);
    Serial.printf("Weight (%s) %.2f g -> HTTP %d\n", pillSize.c_str(), weightG, httpCode);
    http.end();
}

void postPickupStatusToFlask(const String& pillSize, const String& status) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: Wi-Fi Disconnected (pickup status skipped)");
        return;
    }

    HTTPClient http;
    http.setTimeout(4000);
    http.begin(pickupStatusEndpoint);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"pillSize\":\"" + pillSize + "\",\"status\":\"" + status + "\"}";
    int httpCode = http.POST(payload);
    Serial.printf("Pickup (%s) %s -> HTTP %d\n", pillSize.c_str(), status.c_str(), httpCode);
    http.end();
}

void assignNfc(const String& uid) {
    if (uid == "") {
        Serial.println("No NFC UID scanned yet to autofill.");
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: Wi-Fi Disconnected (assignNfc skipped)");
        return;
    }

    HTTPClient http;
    http.setTimeout(4000);
    http.begin(pendingNfcEndpoint);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"uid\":\"" + uid + "\"}";
    int httpCode = http.POST(payload);
    Serial.printf("Autofill NFC %s -> HTTP %d\n", uid.c_str(), httpCode);
    if (httpCode > 0) {
        Serial.println("Server Response: " + http.getString());
    }
    http.end();
}

void checkMSP430UART() {
    while (Serial2.available()) {
        char c = Serial2.read();
        if (c == '\n') {
            uartBuffer.trim();
            if (uartBuffer.startsWith("TEMP:")) {
                float tempC = uartBuffer.substring(5).toFloat();
                postTemperatureToFlask(tempC);
            } else if (uartBuffer.startsWith("DISPENSE:")) {
                String body = uartBuffer.substring(9);
                if (body.startsWith("L:")) {
                    postDispenseStatusToFlask("LARGE", body.substring(2));
                } else if (body.startsWith("S:")) {
                    postDispenseStatusToFlask("SMALL", body.substring(2));
                } else {
                    postDispenseStatusToFlask("SMALL", body);
                }
            } else if (uartBuffer.startsWith("WEIGHT:")) {
                String body = uartBuffer.substring(7);
                if (body.startsWith("L:")) {
                    postWeightToFlask("LARGE", body.substring(2).toFloat());
                } else if (body.startsWith("S:")) {
                    postWeightToFlask("SMALL", body.substring(2).toFloat());
                } else {
                    postWeightToFlask("SMALL", body.toFloat());
                }
            } else if (uartBuffer.startsWith("PICKUP:")) {
                String body = uartBuffer.substring(7);
                if (body.startsWith("L:")) {
                    postPickupStatusToFlask("LARGE", body.substring(2));
                } else if (body.startsWith("S:")) {
                    postPickupStatusToFlask("SMALL", body.substring(2));
                } else {
                    postPickupStatusToFlask("SMALL", body);
                }
            }
            uartBuffer = "";
        } else if (c != '\r') {
            uartBuffer += c;
            if (uartBuffer.length() > 64) {
                uartBuffer = "";
            }
        }
    }
}

void handleNFC() {
    static unsigned long lastScanMs = 0;
    if (millis() - lastScanMs < 1500) {
        return; // Non-blocking 1.5 s debounce between taps
    }

    String scannedUID = get_nfc_card();
    if (scannedUID == "") {
        return;
    }

    lastScanMs = millis();
    Serial.println("Card Tapped! UID: " + scannedUID);

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.setTimeout(2500);
        http.begin(nfcEndpoint);
        http.addHeader("Content-Type", "application/json");

        String jsonPayload = "{\"uid\":\"" + scannedUID + "\"}";
        Serial.println("Verifying with Server: " + jsonPayload);

        int httpResponseCode = http.POST(jsonPayload);
        bool shouldAutofill = false;

        if (httpResponseCode == 200) {
            String response = http.getString();
            if (response.indexOf("\"LARGE\"") >= 0) {
                Serial.println("Server Approved (LARGE)! Sending 'L' to MSP430.");
                Serial2.print('L');
            } else {
                Serial.println("Server Approved (SMALL)! Sending 'S' to MSP430.");
                Serial2.print('S');
            }
        } else {
            Serial.print("Server Denied. HTTP Code: ");
            Serial.println(httpResponseCode);
            String response = http.getString();
            Serial.println("Reason: " + response);

            if (httpResponseCode == 403) {
                shouldAutofill = true;
            }
        }

        http.end();

        if (shouldAutofill) {
            assignNfc(scannedUID);
        }
    } else {
        Serial.println("Error: Wi-Fi Disconnected");
    }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, MSP430_RX_PIN, MSP430_TX_PIN);

    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi! IP Address: ");
    Serial.println(WiFi.localIP());

    // Initialize MFRC522 after Wi-Fi startup current settles
    init_nfc();
}

void loop() {
    handleNFC();
    checkMSP430UART();
    delay(50);
}