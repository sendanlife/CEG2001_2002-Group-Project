# MediTrack Setup Guide

## 1. Install Prerequisites

Required software:

- Python 3.9 or later
- MySQL 8 or a compatible MySQL server
- Visual Studio Code
- PlatformIO IDE extension
- PlatformIO Core and supported board packages
- Git

Optional:

- Node.js and npm
- MySQL Workbench or another database client

## 2. Configure the Database

Start MySQL and create a database:

```sql
CREATE DATABASE pillsExtraction;
```

You can import the supplied reference schema:

```bash
mysql -u root -p pillsExtraction < C:/Users/rache/Documents/embeddedSystems/database/pillsExtraction.sql
```

Alternatively, start Flask with a valid database account and run `db.create_all()` automatically.

Make sure MySQL allows the account from the host running Flask. For local development, the default host is `127.0.0.1`.

## 3. Create the Environment File

Create `.env` in the repository root:

```dotenv
DB_HOST=127.0.0.1
DB_PORT=3306
DB_USER=root
DB_PASSWORD=your-password
DB_NAME=pillsExtraction
PORT=3000
SECRET_KEY=generate-a-random-secret
API_KEY=generate-a-random-api-key
FLASK_ENV=development
```

Generate a secure JWT secret with Python:

```bash
py -c "import secrets; print(secrets.token_hex(32))"
```

Do not reuse the sample secret or API key.

## 4. Install Python Dependencies

From the repository root:

```bash
py -m venv .venv
.venv\Scripts\activate
py -m pip install --upgrade pip
py -m pip install -r requirement.txt
```

The dependency file uses a Unicode UTF-16 encoding. Python's pip can read it in the current environment; if it fails, convert or recreate it as UTF-8.

## 5. Start the Backend

```bash
py app.py
```

Verify:

```bash
curl http://localhost:3000/health
```

Expected response:

```json
{"status":"healthy","service":"meditrack"}
```

## 6. Open the Dashboard

Navigate to:

- `http://localhost:3000/staff/signup`
- `http://localhost:3000/staff/login`
- `http://localhost:3000/staff/dashboard`

Create a staff account first, then sign in.

## 7. Configure and Build the ESP32

Open `ESP32/` as a PlatformIO project.

Update `ESP32/src/main.cpp`:

```cpp
const char* ssid = "your-wifi-name";
const char* password = "your-wifi-password";
const String baseUrl = "http://<server-lan-ip>:3000";
```

Build and upload:

```bash
cd ESP32
pio run
pio run --target upload
pio device monitor
```

Use `monitor_speed = 115200` for the ESP32.

The ESP32 prints:

- Wi-Fi connection status
- Local IP address
- MFRC522 firmware/status
- Server health result
- NFC UID and HTTP responses

## 8. Configure and Build the MSP430

Open `MSP430/` as a PlatformIO project.

Build and upload:

```bash
cd MSP430
pio run
pio run --target upload
pio device monitor
```

Use `monitor_speed = 9600` for the MSP430.

At startup it sends `READY` and then periodically sends temperature/load-cell diagnostics if enabled.

## 9. Connect the Boards

| ESP32 | MSP430 |
|---|---|
| GPIO16 RX | P3.4 TX |
| GPIO17 TX | P3.3 RX |
| GND | GND |

Confirm the pin names against the launch-board schematic and firmware USCI configuration before wiring.

## 10. Test Without Medication

1. Start Flask.
2. Upload ESP32 and MSP430 firmware.
3. Confirm ESP32 server health is reachable.
4. Confirm MSP430 sends `READY`.
5. Confirm the MFRC522 has a valid firmware version.
6. Register a staff account and patient.
7. Assign the scanned NFC UID to the patient.
8. Assign a one-pill dose.
9. Tap the NFC card.
10. Confirm UART receives `S<dose><dose>`.
11. Confirm motor direction and IR detection.
12. Confirm Flask logs `DISPENSE`, weight, and pickup events.

Do not test with medication until mechanical and sensor behavior are reliable.

## 11. Common Issues

### Database Connection Failure

- Confirm MySQL is running.
- Confirm the database exists.
- Confirm credentials and port.
- Confirm `PyMySQL` is installed.
- Recheck pool settings and firewall rules.

### Flask Starts but the ESP32 Is Unreachable

- Find the server LAN IP:

```bash
ipconfig
```

- Update `baseUrl` in the ESP32 source.
- Allow Python/port `3000` through Windows Firewall.
- Ensure both devices are on the same network or routed network.

### No NFC UID

- Confirm 3.3 V power and SPI wiring.
- Check the reader firmware version in serial output.
- Add a delay after startup before continuous polling.
- Confirm the NFC card is compatible with the MFRC522.

### Motor Stops Too Early

- Confirm the IR sensor is receiving reflected/transmitted IR correctly.
- Confirm P2 interrupt lines match `IR1_BIT` and `IR2_BIT`.
- Confirm motor release and coil sequences.
- Increase timeout only after mechanical behavior is understood.

### Incorrect Weight

- Tare the load cell with the cup and tray empty.
- Calibrate using a known weight.
- Adjust `HX711_SCALE_FACTOR`.
- Check for electrical noise and saturation.

## 12. Production Hardening

Before real-world use:

- Replace hard-coded Wi-Fi credentials and server URLs
- Use HTTPS or a secured reverse proxy
- Replace Flask development server with a production WSGI server
- Restrict CORS origins
- Protect hardware endpoints with authentication or a device secret
- Move Flask hardware state to Redis or the database
- Restrict database privileges
- Rotate secrets
- Add request logging and audit records
- Add automated hardware and API tests
- Validate medical workflow and safety requirements with the responsible stakeholders
