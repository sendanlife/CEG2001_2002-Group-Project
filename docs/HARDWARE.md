# MediTrack Hardware Guide

## Hardware Block Diagram

```text
+-----------+  Wi-Fi + HTTP  +-------------+
| ESP32     | <------------> | Flask server|
| + MFRC522 |                 +-------------+
+-----+-----+
      |
      | UART 9600 baud
      v
+---------------------+
| MSP430F5529         |
| temperature ADC     |
| HX711 load cell     |
| IR sensors          |
| stepper outputs     |
+----------+----------+
           |
    motor 1 / motor 2
```

## ESP32

The ESP32 firmware is in `ESP32/src/main.cpp`.

### Key Pins

| Function | ESP32 pin |
|---|---|
| UART RX to MSP430 TX | GPIO16 |
| UART TX to MSP430 RX | GPIO17 |
| MFRC522 SPI SCK | GPIO18 |
| MFRC522 SPI MISO | GPIO19 |
| MFRC522 SPI MOSI | GPIO23 |
| MFRC522 SPI SS | GPIO21 |
| MFRC522 RST | GPIO22 |

These pins are shared between NFC and UART only when no other peripheral uses the same resources.

### ESP32 UART Protocol

The ESP32 listens for newline-terminated messages:

- `TEMP:<value>`: post temperature
- `DISPENSE:<motor>:<dropped>:<expected>`: post dispense status
- `WEIGHT:<grams>`: post small-pill weight
- `LC:*`: debug load-cell line
- `RUN:*` and `REQ:*`: informational UART output

The ESP32 sends:

- `S<dose1><dose2>` to configure both motors
- `1`, `2`, or `D` for legacy/single-character commands

## MSP430

The MSP430 firmware is in `MSP430/src/main.c`.

### UART

The MSP430 initializes USCI_A0 for 9600 baud. UART RX enables dispensing commands, and TX sends status and sensor lines to the ESP32.

The parser recognizes:

| Command | Action |
|---|---|
| `S12` | Set motor 1 to 1 pill and motor 2 to 2 pills |
| `1` | Dispense motor 1 once |
| `2` | Dispense motor 2 once |
| `D` | Dispense one pill from both motors |

### Stepper Motors

The stepper implementation uses stepped GPIO phase sequences.

Motor 1 uses the P1 stepper mask:

- P1.2
- P1.3
- P1.4
- P1.5

Motor 2 uses P6:

- P6.1
- P6.2
- P6.3
- P6.4

Do not connect motor coils directly to MCU pins. Use an appropriate stepper driver or interface circuit with common ground and separated motor power.

### IR Sensors

The IR sensors are attached to P2:

- Motor 1 detection: P2.0
- Motor 2 detection: P2.2

The firmware enables each sensor interrupt only during the motor's dispense sequence. Lockout logic prevents a single falling/rising event from counting as multiple pills.

### Dispense Timeout

The main dispense loop defines `NO_PILL_TIMEOUT_STEPS` as `2500` step iterations. If the loop reaches the timeout before the required count is detected, the MSP430 sends a partial or failed dispense result.

### HX711 Load Cell

HX711 pins are defined in `MSP430/include/loadCell.h`:

- DOUT: P3.6
- SCK: P3.5

Startup operations include:

1. Initialize GPIO
2. Send clock pulses
3. Tare with 16 readings
4. Reset the moving filter
5. Poll for weight changes

The current scale factor is `10.80f`. It must be calibrated for the actual load-cell module and mechanical mounting.

Load-cell status values:

- `LC_OK`
- `LC_NO_DATA`
- `LC_STUCK_LOW`
- `LC_SATURATED`

### Temperature Sensor

The MSP430 temperature sensor implementation uses ADC sampling. The main loop averages ADC readings and sends `TEMP:<tenths-of-degree>` to the ESP32.

The Flask backend alerts for temperatures lower than `15.0` degrees C or higher than `30.0` degrees C.

### Status LEDs and Beeper

The status indicator module is enabled by `USE_STATUS_LEDS 1` in `MSP430/src/main.c`.

States include:

- Initialization
- Dispensing/busy
- Dispensed/success
- Failure
- Beep after successful completion

## Electrical and Mechanical Checklist

- Separate logic and motor power supplies if required by motor current.
- Do not exceed ESP32 or MSP430 GPIO voltage ratings.
- Use 3.3 V logic for the ESP32 and MFRC522.
- Provide common ground between ESP32, MSP430, and sensors.
- Add flyback or driver circuitry required by the stepper motors.
- Secure the load cell mechanically and prevent side loading.
- Shield sensor wiring from motor noise.
- Verify regulator and supply current before energizing motors.
- Keep NFC wiring away from high-current motor traces.

## Wiring Verification

Before dispensing pills:

1. Confirm UART is connected TX to RX across boards.
2. Confirm both UART grounds are common.
3. Confirm the ESP32 Serial2 pins match GPIO16/GPIO17.
4. Confirm the MFRC522 is powered from a supported 3.3 V source.
5. Confirm HX711 DOUT/SCK pins match P3.6/P3.5.
6. Confirm IR sensor direction and pull configuration.
7. Confirm motor coil assignments and expected rotation direction.
8. Test motors without pills first.
9. Calibrate and tare the load cell.
10. Confirm `READY` appears on the UART monitor.

## Calibration

### Load Cell

Place a known calibration weight on the tray and read the raw average output. Calculate or adjust `HX711_SCALE_FACTOR` until the firmware reports approximately the true weight.

The zero offset is set by `LoadCell_Tare(16)` at startup, so the tray should be unloaded and stable before `main()` reaches that line.

### Temperature

Verify the ADC conversion and sensor implementation against a known reference. Update the conversion formula in `MSP430/src/main.c` or `tempSensor.c` if the sensor differs.

### Pill Profiles

Small and large pill profiles are currently fixed in `app.py`:

- `SMALL`: `0.50 g`
- `LARGE`: `1.20 g`

Update these values after measuring the physical pill type.
