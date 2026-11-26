# MediTrack Smart Medication Dispenser

MediTrack is an embedded medication-dispenser project with a Flask/MySQL backend and a staff dashboard. It combines an ESP32, an MSP430F5529 LaunchPad, NFC authentication, temperature and weight sensing, stepper motors, IR pill detection, and a live web dashboard.

The system identifies a patient using an MFRC522 NFC card, retrieves that patient's assigned medication doses from MySQL, sends the doses to the MSP430, dispenses up to two pill types, verifies the result, and reports the outcome to the dashboard.

## Features

- Staff signup and login using JWT bearer authentication
- Patient registration and assignment to a staff account
- Pill catalogue management
- Patient pill inventory and dose assignment (Gabriel)
- NFC patient authentication
- Live temperature reporting
- Two-motor pill dispensing (Gabriel)
- IR-based pill-drop detection
- HX711 load-cell weight verification
- Cup pickup and missed-pickup reporting
- Real-time Socket.IO dashboard events
- Dispensing quantity and dispense-history logging

## System Architecture

```text
                    +----------------------+
Patient/NFC card -> | ESP32 + MFRC522      |
                    | Wi-Fi / HTTP / UART  |
                    +----------+-----------+
                               |
              UART 9600 baud   |   HTTP JSON
                               v
                    +----------------------+
                    | MSP430F5529          | <-> Two stepper motors
                    | sensors / motors     | <-> IR pill sensors
                    | temperature / HX711  | <-> HX711 load cell
                    +----------+-----------+
                               |
                               v
                    +----------------------+
                    | Flask / Socket.IO    |
                    | REST API             |
                    +----------+-----------+
                               |
                  +------------+------------+
                  |                         |
                  v                         v
           Staff dashboard             MySQL database
        HTML / JavaScript            SQLAlchemy models
```

The ESP32 is the network bridge. It verifies NFC IDs, requests patient dose settings, and forwards MSP430 sensor and dispensing events to Flask over HTTP. The MSP430 performs real-time hardware control and sends newline-delimited messages back over UART.

## Repository Layout

```text
.
|-- app.py                 Flask application and hardware API endpoints
|-- config.py              Environment-backed server configuration
|-- requirement.txt        Python dependencies
|-- package.json           Socket.IO client dependency
|-- routes/                Flask REST blueprints
|-- models/                SQLAlchemy database models
|-- database/              Database initialization and SQL schema
|-- utils/                 Authentication and API response helpers
|-- static/                Staff login, signup, and dashboard pages
|-- ESP32/                 ESP32 PlatformIO firmware
|   |-- src/main.cpp       Wi-Fi, NFC, HTTP, and UART orchestration
|   |-- src/nfc.cpp        MFRC522 low-level NFC handling
|   `-- src/esp32uart.cpp  ESP32 UART helper
`-- MSP430/                MSP430 PlatformIO firmware
    |-- src/main.c         Dispenser control loop
    |-- src/msp430uart.c  MSP430 UART command protocol
    |-- src/stepper.c     Bipolar/unipolar step sequences
    |-- src/irSensor.c    IR pill detection
    |-- src/loadCell.c    HX711 readings and averaging
    `-- src/tempSensor.c  ADC temperature sampling
```

## Requirements

### Server

- Windows, Linux, or macOS
- Python 3.9 or later
- MySQL server
- A TCP network connection between the ESP32 and server
- Optional Node.js tooling if managing the npm client dependency

### Hardware

- ESP32 development board
- MFRC522 NFC reader and compatible NFC card/tag
- Texas Instruments MSP430F5529 LaunchPad
- Two stepper motors and suitable driver/control circuitry
- Two IR pill-detection sensors
- HX711 load-cell amplifier
- Load cell
- Temperature sensor
- LEDs/beeper and supporting circuitry

## Quick Start

### 1. Configure MySQL

Create a database and load the supplied SQL schema:

```bash
mysql -u root -p
SOURCE C:/path/to/embeddedSystems/database/pillsExtraction.sql;
```

Alternatively, let Flask SQLAlchemy create the tables when `create_app()` calls `db.create_all()`. The SQL file remains useful for a manual database setup or reference schema.

### 2. Configure environment variables

Create a `.env` file in the repository root:

```dotenv
DB_HOST=127.0.0.1
DB_PORT=3306
DB_USER=root
DB_PASSWORD=replace-with-your-password
DB_NAME=pillsExtraction
PORT=3000
SECRET_KEY=replace-with-a-long-random-secret
API_KEY=replace-with-a-long-random-api-key
FLASK_ENV=development
```

Never commit `.env`, passwords, API keys, or production secrets.

### 3. Install and run the backend

```bash
cd C:\Users\rache\Documents\embeddedSystems
py -m pip install -r requirement.txt
py app.py
```

The server listens on `0.0.0.0:<PORT>`. The default port is `3000`.

Open:

- Staff login: `http://localhost:3000/staff/login`
- Staff signup: `http://localhost:3000/staff/signup`
- Staff dashboard: `http://localhost:3000/staff/dashboard`
- Health check: `http://localhost:3000/health`

### 4. Build the ESP32 firmware

Open `ESP32/` in VS Code with PlatformIO installed:

```bash
cd ESP32
pio run
pio run --target upload
pio device monitor
```

Before uploading, update the Wi-Fi credentials and server address in `ESP32/src/main.cpp`:

```cpp
const char* ssid = "your-wifi-name";
const char* password = "your-wifi-password";
const String baseUrl = "http://<server-ip>:3000";
```

The ESP32 and server must be able to reach each other on the same network or through an explicitly configured route.

### 5. Build the MSP430 firmware

Open `MSP430/` in VS Code with PlatformIO installed:

```bash
cd MSP430
pio run
pio run --target upload
pio device monitor
```

The firmware is configured for the `lpmsp430f5529` board and a 9600 baud UART monitor.

## Normal Dispensing Flow

1. A staff member registers or opens a patient and assigns NFC/pill information.
2. The staff dashboard opens a live Socket.IO connection.
3. A patient taps an NFC card on the MFRC522 reader.
4. The ESP32 sends the card UID to `POST /api/auth/verify-nfc`.
5. Flask resolves the UID to a patient and stores it as the latest dispensing patient.
6. The ESP32 requests assigned pill doses from `GET /api/pills/patient/<uuid>/count`.
7. The first assigned pill maps to motor 1 and the second assigned pill maps to motor 2.
8. The ESP32 sends an `S<motor1Dose><motor2Dose>` UART message to the MSP430.
9. The MSP430 steps each motor and counts passing pills with its IR sensors.
10. The MSP430 sends `DISPENSE:<motor>:<dropped>:<expected>` and an averaged load-cell weight to the ESP32.
11. The ESP32 posts those values to the Flask hardware endpoints.
12. Flask decrements inventory, adds dispense logs, verifies the weight, and emits Socket.IO events.
13. The dashboard updates the current patient, cup state, temperature, weight, and event feed.

## Motor and Dose Mapping

The API treats assigned pill order by `pillsId` ascending:

| Hardware motor | Assigned inventory position | Message field |
|---|---|---|
| Motor 1 | First user-pill row | `smallPill` / `dose1` |
| Motor 2 | Second user-pill row | `bigPill` / `dose2` |

The MSP430 UART parser accepts an `S` command followed by two one-digit doses, for example `S21`. Dose values above 9 are clamped by the ESP32 before transmission.

## Dispensing Validation

The Flask weight endpoint uses pill profiles:

| Pill size | Nominal pill weight |
|---|---|
| `SMALL` | 0.50 g |
| `LARGE` | 1.20 g |

A weight is considered verified when it meets 50% of the nominal pill weight. The estimated pill count is the measured weight divided by the profile weight, rounded to the nearest integer.

The dispense-status endpoint reports:

- `SUCCESS`: dropped pills equal or exceed the expected count
- `UNSUCCESSFUL`: some pills were dropped but fewer than expected
- `FAILED`: no pills were dropped

## API Summary

All JSON application endpoints use the `success`/`error` response envelope from `utils/responses.py`.

```json
{
  "success": true,
  "message": "OK",
  "data": {}
}
```

Common endpoint groups:

| Area | Example endpoints |
|---|---|
| Health | `GET /health` |
| Staff | `/api/staff/signup`, `/api/staff/login`, `/api/staff/me`, `/api/staff/dashboard` |
| Patients | `/api/staff/patients`, `/api/staff/patients/<uuid>`, `/api/staff/patients/<uuid>/pills` |
| Pills | `/api/pills`, `/api/users/<uuid>/pills`, `/api/pills/patient/<uuid>/count` |
| NFC | `/api/auth/verify-nfc`, `/api/auth/assign-nfc`, `/api/auth/pending-nfc` |
| Hardware | `/api/temperature`, `/api/dispense-status`, `/api/weight`, `/api/pickup-status`, `/api/hardware/latest` |

See [Technical Architecture](docs/ARCHITECTURE.md), [API Reference](docs/API.md), [Hardware Guide](docs/HARDWARE.md), [Database Reference](docs/DATABASE.md), and [Setup Guide](docs/SETUP.md).

## Authentication

Staff credentials are hashed with Werkzeug's PBKDF2-SHA256 implementation. Staff login creates a signed JWT with a default 24-hour expiry. Protected staff endpoints require:

```text
Authorization: Bearer <JWT>
```

The JWT payload contains the staff UUID and `role=staff`. NFC verification is a separate hardware authentication mechanism and does not use a patient JWT.

## Real-Time Dashboard Events

Socket.IO is initialized by the Flask application and the dashboard connects to the same host. Hardware endpoints emit `new-event` messages containing event types such as:

- `TEMP_UPDATE`
- `TEMP_ALERT`
- `PILL_DISPENSED`
- `DISPENSE_ERROR`
- `DISPENSE_VERIFIED`
- `DISPENSE_WEIGHT_FAIL`
- `PILL_TAKEN`
- `PILL_MISSED`
- `NFC_SCANNED`

## Development Notes

- `app.py` creates database tables on startup and initializes Socket.IO.
- Flask CORS allows all origins for `/api/*`.
- Socket.IO allows all origins.
- Firmware networking values are currently embedded in source files; move them to build-time configuration for production deployments.
- Temperature alert thresholds are below 15 degrees C or above 30 degrees C.
- `.env` is ignored by Git.
- Generated PlatformIO `.pio` output directories are ignored.
- The load-cell calibration factor must be measured and adjusted for the physical hardware.

## Troubleshooting

### The ESP32 cannot reach Flask

- Confirm the server IP is reachable from the ESP32.
- Confirm the PC firewall allows TCP port `3000`.
- Set `baseUrl` to the PC's current LAN IP, not `localhost`.
- Check that Flask listens on `0.0.0.0`, not only a loopback interface.

### NFC card is unauthorized

- Confirm the UID is assigned in the `users.NFCId` field.
- Tap the card and use the pending-NFC workflow to prefill a registration field.
- The UID is normalized to uppercase in the ESP32 and backend.

### The load cell reports invalid readings

- Verify HX711 power, ground, DOUT, SCK, and load-cell bridge wiring.
- Tare the load cell during startup.
- Use a known weight to calibrate `HX711_SCALE_FACTOR` in `MSP430/include/loadCell.h`.
- Confirm the physical load-cell scale is stable and not electrically saturated.

### No pills are detected

- Verify motor and IR sensor wiring.
- Check that the IR sensor interrupt pins are not shorted to ground.
- Confirm assigned doses are greater than zero.
- Inspect UART output for `DISPENSE:<motor>:<dropped>:<expected>`.

## License

See [LICENSE.md](LICENSE.md).
