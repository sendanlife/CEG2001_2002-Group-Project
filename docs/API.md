# MediTrack API Reference

The API is served by Flask, normally on port `3000`.

## Response Format

Successful responses:

```json
{
  "success": true,
  "message": "Description",
  "data": {}
}
```

Error responses:

```json
{
  "success": false,
  "message": "Description",
  "details": null
}
```

Details are omitted or `null` by most routes.

## Authentication

Protected staff routes expect:

```http
Authorization: Bearer <JWT>
```

The JWT is returned by staff login or signup. It expires after 24 hours by default.

## Service

### Health

```http
GET /health
```

Response:

```json
{
  "status": "healthy",
  "service": "meditrack"
}
```

## Staff Authentication

### Sign Up

```http
POST /api/staff/signup
Content-Type: application/json
```

```json
{
  "name": "Jane Smith",
  "email": "jane@example.com",
  "password": "A-strong-password",
  "phoneNumber": "91234567"
}
```

### Login

```http
POST /api/staff/login
Content-Type: application/json
```

```json
{
  "email": "jane@example.com",
  "password": "A-strong-password"
}
```

Returns a JWT and staff object.

### Current Staff

```http
GET /api/staff/me
Authorization: Bearer <JWT>
```

### Dashboard Summary

```http
GET /api/staff/dashboard
Authorization: Bearer <JWT>
```

Returns staff details, assigned patient count, and patient summary fields.

## Staff Patient Management

### Add Patient

```http
POST /api/staff/patients
Authorization: Bearer <JWT>
Content-Type: application/json
```

```json
{
  "name": "Alex Tan",
  "email": "alex@example.com",
  "nric": "S1234567A",
  "phoneNumber": "91234567",
  "NFCid": "04A1B2C3"
}
```

### Get Patient

```http
GET /api/staff/patients/<uuid>
Authorization: Bearer <JWT>
```

Returns patient identity, inventory, dispenser totals, and latest temperature.

### Delete Patient

```http
DELETE /api/staff/patients/<uuid>
Authorization: Bearer <JWT>
```

## Pill Management

### List Pills

```http
GET /api/pills
```

### Get Pill

```http
GET /api/pills/<pillsId>
```

### Create Pill

```http
POST /api/pills
Content-Type: application/json
```

```json
{
  "pillsId": "PIL01",
  "pillName": "Example tablet",
  "DESCRIPTION": "Optional description"
}
```

### Update Pill

```http
PUT /api/pills/<pillsId>
Content-Type: application/json
```

```json
{
  "pillName": "Updated name",
  "DESCRIPTION": "Updated description"
}
```

### Delete Pill

```http
DELETE /api/pills/<pillsId>
```

## Patient Inventory

### List Assigned Pills

```http
GET /api/users/<uuid>/pills
```

### Assign a Pill

```http
POST /api/users/<uuid>/pills
Content-Type: application/json
```

```json
{
  "pillsId": "PIL01",
  "quantity": 30,
  "pillsDose": 2
}
```

Staff can also update an existing assignment using:

```http
POST /api/staff/patients/<uuid>/pills
Authorization: Bearer <JWT>
Content-Type: application/json
```

### Retrieve Hardware Dose Counts

```http
GET /api/pills/patient/<uuid>/count
```

Example:

```json
{
  "success": true,
  "message": "Pill count retrieved successfully.",
  "data": {
    "uuid": "A1234",
    "assignedPillCount": 2,
    "smallPill": 2,
    "bigPill": 1
  }
}
```

The ESP32 sends `smallPill` and `bigPill` as dose values for motor 1 and motor 2.

## NFC

### Verify NFC Card

```http
POST /api/auth/verify-nfc
Content-Type: application/json
```

```json
{
  "uid": "04A1B2C3"
}
```

Responses:

- `200`: UID belongs to a patient
- `403`: UID is not registered

### Assign NFC

```http
POST /api/auth/assign-nfc
Content-Type: application/json
```

```json
{
  "uuid": "A1234",
  "nfcId": "04A1B2C3"
}
```

### Save/Fetch Pending NFC

```http
POST /api/auth/pending-nfc
Content-Type: application/json
```

```json
{
  "uid": "04A1B2C3"
}
```

```http
GET /api/auth/pending-nfc
```

## Hardware Ingestion

These endpoints are called by the ESP32 gateway.

### Temperature

```http
POST /api/temperature
Content-Type: application/json
```

```json
{
  "temperature": 24.5,
  "userId": "A1234"
}
```

An alert is raised below `15.0` degrees C or above `30.0` degrees C.

### Dispense Status

```http
POST /api/dispense-status
Content-Type: application/json
```

```json
{
  "uuid": "A1234",
  "motor": 1,
  "dropped": 2,
  "expected": 2,
  "pillSize": "SMALL"
}
```

Valid status values are calculated as:

- `SUCCESS`: `dropped >= expected`
- `UNSUCCESSFUL`: `0 < dropped < expected`
- `FAILED`: `dropped <= 0`

### Weight

```http
POST /api/weight
Content-Type: application/json
```

```json
{
  "pillSize": "SMALL",
  "weight": 1.02
}
```

### Pickup Status

```http
POST /api/pickup-status
Content-Type: application/json
```

```json
{
  "pillSize": "SMALL",
  "status": "TAKEN"
}
```

Allowed status values are `TAKEN` and `TIMEOUT`.

### Latest Hardware State

```http
GET /api/hardware/latest
```

Returns the Flask process's latest hardware-state object, including temperature, alert status, cup status, pill size, weight, patient name, and recent events.
