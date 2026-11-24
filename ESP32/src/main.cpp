#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "nfc.h"

#define MSP430_RX_PIN 16
#define MSP430_TX_PIN 17

const char* ssid = "";
const char* password = "";

const String baseUrl = "http://<IP_ADDRESS>:3000";
const String nfcEndpoint = baseUrl + "/api/auth/verify-nfc";
const String tempEndpoint = baseUrl + "/api/temperature";
const String dispenseStatusEndpoint = baseUrl + "/api/dispense-status";
const String weightEndpoint = baseUrl + "/api/weight";
const String pendingNfcEndpoint = baseUrl + "/api/auth/pending-nfc";
String lastScannedUID = "";
bool assignNfcMode = false;

String uartBuffer = "";

void postTemperatureToFlask(float tempC) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: Wi-Fi Disconnected (temp skipped)");
        return;
    }

    HTTPClient http;
    http.setTimeout(3000);
    http.begin(tempEndpoint);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"temperature\":" + String(tempC, 1) + "}";
    int httpCode = http.POST(payload);
    Serial.printf("Temp %.1f C -> HTTP %d\n", tempC, httpCode);
    http.end();
}

void postDispenseStatusToFlask(const String& status) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: Wi-Fi Disconnected (dispense status skipped)");
        return;
    }

    HTTPClient http;
    http.setTimeout(3000);
    http.begin(dispenseStatusEndpoint);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"status\":\"" + status + "\"}";
    int httpCode = http.POST(payload);
    Serial.printf("Dispense %s -> HTTP %d\n", status.c_str(), httpCode);
    http.end();
}

void postWeightToFlask(float weightG) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: Wi-Fi Disconnected (weight skipped)");
        return;
    }

    HTTPClient http;
    http.setTimeout(3000);
    http.begin(weightEndpoint);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"weight\":" + String(weightG, 1) + "}";
    int httpCode = http.POST(payload);
    Serial.printf("Weight %.1f g -> HTTP %d\n", weightG, httpCode);
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
                String status = uartBuffer.substring(9);
                postDispenseStatusToFlask(status);
            } else if (uartBuffer.startsWith("WEIGHT:")) {
                float weightG = uartBuffer.substring(7).toFloat();
                postWeightToFlask(weightG);
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
    http.setTimeout(3000);
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

void handleNFC() {
    String scannedUID = get_nfc_card();

    if (scannedUID != "") {
        scannedUID.trim();
        scannedUID.toUpperCase();
        Serial.println("Card Tapped! UID: " + scannedUID);

        // Only push for autofill when a new card is scanned
        if (scannedUID != lastScannedUID) {
            lastScannedUID = scannedUID;
            assignNfc(scannedUID);
        }

        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.setTimeout(3000);
            http.begin(nfcEndpoint);
            http.addHeader("Content-Type", "application/json");

            String jsonPayload = "{\"uid\":\"" + scannedUID + "\"}";
            Serial.println("Verifying with Server: " + jsonPayload);

            int httpResponseCode = http.POST(jsonPayload);

            if (httpResponseCode == 200) {
                Serial.println("Server Approved! Sending 'D' to MSP430.");
                Serial2.print('D');
            } else {
                Serial.print("Server Denied. HTTP Code: ");
                Serial.println(httpResponseCode);
                String response = http.getString();
                Serial.println("Reason: " + response);
            }

            http.end();
        } else {
            Serial.println("Error: Wi-Fi Disconnected");
        }

        delay(3000);
    }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, MSP430_RX_PIN, MSP430_TX_PIN);

    init_nfc();

    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi! IP Address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    checkMSP430UART();
    handleNFC();
    delay(50);
}