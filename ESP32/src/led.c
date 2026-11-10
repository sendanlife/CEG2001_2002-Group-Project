#include "led.h"
#include <Arduino.h>

#define LED_RED_PIN 2 // ESP32 GPIO2

void LED_Init(void) {
    pinMode(LED_RED_PIN, OUTPUT);
    digitalWrite(LED_RED_PIN, LOW); // Start with LED off
}

void LED_TurnOn(void) {
    digitalWrite(LED_RED_PIN, HIGH);
}

void LED_TurnOff(void) {
    digitalWrite(LED_RED_PIN, LOW);
}