# MediTrack Technical Architecture

## Purpose

MediTrack combines an embedded dispenser, a network gateway, a Flask application, and a staff-facing browser dashboard. It separates hardware timing-critical work from web/database work:

- The MSP430 controls motors and reads local sensors.
- The ESP32 handles NFC, Wi-Fi, HTTP, and UART bridging.
- Flask validates requests, manages persistence, and broadcasts dashboard events.
- MySQL stores staff, patient, medication, inventory, and dispense data.

## Runtime Components

### Staff Browser

The staff dashboard is implemented as a static HTML page with vanilla JavaScript. It uses:

- `fetch()` for REST API calls
- Local storage for JWT and staff metadata
- Socket.IO for live hardware events
- Modal pages for patient, pill, and dose management

### Flask Application

`app.py` creates the Flask application, initializes SQLAlchemy, registers route blueprints, configures CORS, initializes Socket.IO, and defines hardware ingestion endpoints.

The application is created through `create_app()`. `app.py` also creates database tables with `db.create_all()` and starts the Socket.IO server when executed directly.

### Route Blueprints

| Blueprint | Responsibility |
|---|---|
| `auth_bp` | Patient login and NFC verification |
| `users_bp` | General patient CRUD and NFC lookup |
| `pills_bp` | Pill catalogue and patient inventory |
| `staffauth_bp` | Staff authentication and dashboard operations |

### Database

Flask-SQLAlchemy maps Python models to MySQL tables. `database/connection.py` creates the SQLAlchemy object and configures pool pre-ping and recycling behavior.

The application keeps some live hardware state in memory and sends temperature and dispense events to the browser. Database-backed state should be used for durable records; process memory is not shared across multiple Flask workers.

## MCU Responsibilities

### ESP32

The ESP32 is the network gateway and NFC controller.

- Connects to Wi-Fi
- Polls the MFRC522
- Normalizes NFC UIDs
- Calls Flask REST endpoints over HTTP
- Stores the latest patient UUID and latest NFC UID in Flask process configuration
- Parses newline-delimited UART messages from the MSP430
- Sends dispense, weight, temperature, and pickup events to Flask

The ESP32 Serial1 configuration is not used for this bridge. It uses `Serial2` at 9600 baud on pins 16 and 17.

### MSP430

The MSP430 is responsible for:

- UART command parsing
- Stepper motor sequence generation
- IR interrupt-based pill counting
- Temperature ADC sampling
- HX711 polling and averaging
- Status LEDs and beeper output
- Low-power sleep transitions

The MSP430 uses UART command `S<dose1><dose2>` to receive doses. It may also receive single-character `1`, `2`, or `D` commands and a button-triggered fallback.

## Communication Protocols

### HTTP

The ESP32 sends JSON requests to the Flask server. All JSON payloads use `Content-Type: application/json`.

Important hardware endpoints:

| Direction | Endpoint | Payload |
|---|---|---|
| ESP32 -> Flask | `POST /api/auth/verify-nfc` | `{ "uid": "..." }` |
| ESP32 -> Flask | `GET /api/pills/patient/<uuid>/count` | No body |
| MSP430 -> ESP32 -> Flask | `POST /api/temperature` | `{ "temperature": 22.5 }` |
| MSP430 -> ESP32 -> Flask | `POST /api/dispense-status` | `{ "motor": 1, "dropped": 2, "expected": 2, "uuid": "..." }` |
| MSP430 -> ESP32 -> Flask | `POST /api/weight` | `{ "pillSize": "SMALL", "weight": 1.01 }` |
| MSP430 -> ESP32 -> Flask | `POST /api/pickup-status` | `{ "pillSize": "SMALL", "status": "TAKEN" }` |

### UART

The MSP430 and ESP32 communicate at 9600 baud, 8 data bits, no parity, 1 stop bit.

ESP32 to MSP430:

```text
S21
```

Where:

- `S` starts a dose command
- `2` is motor 1's dose
- `1` is motor 2's dose

MSP430 to ESP32:

```text
READY
TEMP:22.5
RAW:2048
RUN:1:2
DISPENSE:1:2:2
WEIGHT:1.0
LC:OK raw=123 counts=456 g*10=10
```

`TEMP:`, `DISPENSE:`, and `WEIGHT:` are handled by the ESP32 orchestration code.

### Socket.IO

Flask emits `new-event` messages. The browser registers a `new-event` listener and updates the live monitor. This is suitable for the dashboard but does not replace HTTP storage or acknowledgement paths.

## Dispensing State Flow

```text
[NFC absent]
  -> card tap
  -> verify UID
  -> authorized patient?
       no  -> HTTP 403, save pending UID for staff assignment
       yes -> retrieve assigned doses
             -> send S<dose1><dose2> over UART

[MSP430 receives S command]
  -> set motor flags/counts
  -> motor 1 dispense loop
  -> motor 2 dispense loop
  -> count pills with IR sensors
  -> read load-cell weight
  -> send DISPENSE and WEIGHT lines

[Flask receives hardware status]
  -> update inventory quantity
  -> add dispense log
  -> add dispense history
  -> emit Socket.IO event

[Weight verification]
  -> verified or weight fail
  -> update cup state
  -> await pickup status
       TAKEN -> PILL_TAKEN
       TIMEOUT -> PILL_MISSED
```

## Persistence Model

Important state categories:

- Staff and patients: MySQL
- Pill catalogue: MySQL
- Per-patient inventory and dose: MySQL
- Successful/unsuccessful dispense detail: MySQL
- Historical NFC/status entries: MySQL `dispense_history`
- Latest NFC, active patient UUID, latest temperature: Flask process memory
- Browser live event feed: browser and Socket.IO state

The process-memory fields are intended for this deployment and should be moved to a shared store or database when running multiple Flask workers.

## Failure and Recovery Behavior

### ESP32

- If Wi-Fi is lost, HTTP posts are skipped and printed to serial.
- RFID availability is checked every five seconds and reinitialized if needed.
- Repeated card reads are throttled by a 1.5-second delay.

### Flask

- Invalid JSON or missing numeric fields return HTTP 400.
- NFC IDs not in MySQL return HTTP 403.
- Missing patient records return appropriate 404 responses.
- Database modifications are rolled back on exception.

### MSP430

- Dispensing has a step timeout to prevent infinite motor operation.
- A brief extra IR check runs after the main loop.
- Load-cell readings have no-data, stuck-low, and saturation status checks.
- The motor is released after each dispense sequence.

## Configuration Boundaries

Move these values to external or build-time configuration when hardening the deployment:

- Wi-Fi SSID and password
- Flask server base IP
- Database credentials
- JWT secret
- API key
- UART pins
- NFC SPI pins
- Temperature thresholds
- Load-cell calibration factor
- Pill nominal weights
