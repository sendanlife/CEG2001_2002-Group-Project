#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "nfc.h"

#define MSP430_RX_PIN 16   // ESP32 RX2  <- MSP430 P3.3 (TXD)
#define MSP430_TX_PIN 17   // ESP32 TX2  -> MSP430 P3.4 (RXD)

// 1 = link test: send '1' every 2 s and print every raw byte from the MSP430
//     (card taps are ignored in this mode)
// 0 = normal operation
#define UART_LINK_TEST 1

const char* ssid = "reishell";
const char* password = "020503Tao";

const String baseUrl = "http://192.168.43.52:3000";
const String nfcEndpoint = baseUrl + "/api/auth/verify-nfc";
const String tempEndpoint = baseUrl + "/api/temperature";
const String dispenseStatusEndpoint = baseUrl + "/api/dispense-status";
const String weightEndpoint = baseUrl + "/api/weight";
const String pendingNfcEndpoint = baseUrl + "/api/auth/pending-nfc";
const String assignPillCountEndpoint = baseUrl + "/api/pills/patient/{uuid}/count";
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
            } else if (uartBuffer.length() > 0) {
                // anything else (HELLO, GOT, ACK...) is shown, not dropped
                Serial.println("MSP430 says: " + uartBuffer);
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

String extractJsonString(const String& json, const String& key) {
    String search = "\"" + key + "\"";
    int idx = json.indexOf(search);
    if (idx == -1) return "";

    int colonIdx = json.indexOf(':', idx + search.length());
    if (colonIdx == -1) return "";

    int firstQuote = json.indexOf('"', colonIdx + 1);
    if (firstQuote == -1) return "";

    int secondQuote = json.indexOf('"', firstQuote + 1);
    if (secondQuote == -1) return "";

    return json.substring(firstQuote + 1, secondQuote);
}

int extractJsonInt(const String& json, const String& key) {
    String search = "\"" + key + "\"";
    int idx = json.indexOf(search);
    if (idx == -1) return -1;

    int colonIdx = json.indexOf(':', idx + search.length());
    if (colonIdx == -1) return -1;

    int start = colonIdx + 1;
    while (start < (int)json.length() && (json[start] == ' ' || json[start] == '\t')) {
        start++;
    }

    String numStr = "";
    while (start < (int)json.length() && isDigit(json[start])) {
        numStr += json[start];
        start++;
    }

    return (numStr.length() > 0) ? numStr.toInt() : -1;
}

int assignPillCountToPatient(const String& patientUUID) {
    if (patientUUID == "") {
        Serial.println("No patient UUID provided for pill count assignment.");
        return -1;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: Wi-Fi Disconnected (assignPillCount skipped)");
        return -1;
    }

    HTTPClient http;
    http.setTimeout(3000);
    String endpoint = assignPillCountEndpoint;
    endpoint.replace("{uuid}", patientUUID);
    http.begin(endpoint);

    int httpCode = http.GET();
    Serial.printf("Fetching assigned pill count for UUID %s -> HTTP %d\n", patientUUID.c_str(), httpCode);

    int pillCount = -1;
    if (httpCode == 200) {
        String response = http.getString();
        Serial.println("Assigned Pill Count Response: " + response);
        pillCount = extractJsonInt(response, "assignedPillCount");
    } else {
        Serial.println("Failed to fetch assigned pill count. HTTP Code: " + String(httpCode));
    }

    http.end();
    return pillCount;
}

// Sends one command byte to the MSP430 and logs it
void sendToMSP430(char cmd) {
    Serial2.write((uint8_t)cmd);
    Serial2.flush();   // wait until the byte has physically left the pin
    Serial.printf("TX -> MSP430: '%c' (0x%02X)\n", cmd, (uint8_t)cmd);
}

void handleNFC() {
    String scannedUID = get_nfc_card();

    if (scannedUID != "") {
        scannedUID.trim();
        scannedUID.toUpperCase();
        Serial.println("Card Tapped! UID: " + scannedUID);

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
            String response = http.getString();

            if (httpResponseCode == 200) {
                String patientUUID = extractJsonString(response, "uuid");
                int pillCount = assignPillCountToPatient(patientUUID);

                Serial.printf("DEBUG: Patient UUID: %s, Pill Count: %d\n", patientUUID.c_str(), pillCount);

                if (pillCount == 1) {
                    Serial.println("DEBUG: 1 pill assigned -> Sending '1' to MSP430.");
                    sendToMSP430('1');
                } else if (pillCount >= 2) {
                    Serial.printf("DEBUG: 2+ pills assigned -> Sending 'D' to MSP430. Pill Count: %d\n", pillCount);
                    sendToMSP430('D');
                } else {
                    Serial.println("DEBUG: No pills assigned (0) -> Nothing sent.");
                }
            } else {
                Serial.print("Server Denied. HTTP Code: ");
                Serial.println(httpResponseCode);
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

#if UART_LINK_TEST

void loop() {
    static unsigned long lastSend = 0;

    // send '1' every 2 s; MSP430 red LED should toggle each time
    if (millis() - lastSend >= 2000) {
        lastSend = millis();
        sendToMSP430('1');
    }

    // dump every raw byte coming from the MSP430
    while (Serial2.available()) {
        uint8_t b = Serial2.read();
        Serial.printf("RX <- MSP430: 0x%02X '%c'\n", b, (b >= 32 && b < 127) ? (char)b : '.');
    }
}

#else

void loop() {
    checkMSP430UART();
    handleNFC();
    delay(50);
}

#endif
