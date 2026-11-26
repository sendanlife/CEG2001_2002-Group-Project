# MediTrack Database Reference

## Database

The default SQLAlchemy URI is constructed in `config.py`:

```text
mysql+pymysql://<DB_USER>:<DB_PASSWORD>@<DB_HOST>:<DB_PORT>/<DB_NAME>
```

The default database name is `pillsExtraction`.

## Tables

### `medicalStaff`

Medical staff account and identity table.

| Column | SQL type | Constraints |
|---|---|---|
| `MSId` | `CHAR(5)` | Primary key |
| `NAME` | `VARCHAR(32)` | Not null |
| `email` | `VARCHAR(128)` | Not null, unique |
| `phoneNumber` | `VARCHAR(20)` | Not null |
| `passwordHash` | `VARCHAR(225)` | Not null |

### `users`

Patient account and NFC identity table.

| Column | SQL type | Constraints |
|---|---|---|
| `UUID` | `CHAR(5)` | Primary key |
| `NAME` | `VARCHAR(32)` | Not null |
| `email` | `VARCHAR(128)` | Not null, unique |
| `nric` | `VARCHAR(9)` | Not null, unique |
| `phoneNumber` | `VARCHAR(20)` | Not null |
| `MSId` | `CHAR(5)` | Foreign key to `medicalStaff.MSId` |
| `NFCId` | `CHAR(20)` | Unique |
| `medicalHistoryId` | `CHAR(5)` | Associated medical record identifier |

Deleting a staff member sets associated patient `MSId` values to `NULL` under the supplied SQL schema.

### `medical`

Medical record metadata.

| Column | SQL type | Constraints |
|---|---|---|
| `medicalHistoryId` | `CHAR(5)` | Primary key |
| `UUID` | `CHAR(5)` | Foreign key to `users.UUID`, cascade |
| `issueBy` | `CHAR(5)` | Foreign key to `medicalStaff.MSId`, cascade |
| `medicalStaffName` | `VARCHAR(32)` | Not null |

### `pill`

Pill catalogue.

| Column | SQL type | Constraints |
|---|---|---|
| `pillsId` | `CHAR(5)` | Primary key |
| `pillName` | `CHAR(32)` | Not null |
| `DESCRIPTION` | `TEXT` | Nullable |

### `userPill`

Patient/pill assignment and inventory.

| Column | SQL type | Constraints |
|---|---|---|
| `UUID` | `CHAR(5)` | Composite primary key, user foreign key |
| `pillsId` | `CHAR(5)` | Composite primary key, pill foreign key |
| `quantity` | `INT` | Not null, default `0` |
| `pillsDose` | `INT` | Not null, default `1` |

`quantity` is the remaining inventory. `pillsDose` is the count to send to the matching hardware motor.

### `dispenseLog`

Detailed dispense result entries.

| Column | SQL type | Constraints |
|---|---|---|
| `dispenseId` | Integer | SQLAlchemy primary key |
| `UUID` | `VARCHAR(5)` | Foreign key, cascade |
| `pillsId` | `VARCHAR(5)` | Foreign key |
| `amountDispense` | `INT` | Default `0` |
| `status` | `BOOLEAN` | Default `False` |
| `timestamp` | `DATETIME/TIMESTAMP` | Default current timestamp |

The SQLAlchemy model uses an integer auto-incrementing primary key. The standalone SQL script declares `dispenseId CHAR(5)` but should be regarded as a reference schema; execute the SQLAlchemy `create_all()` path for consistent model behavior.

### `dispense_history`

Additional status history created by `DispenseHistory` in `database/connection.py`.

| Column | SQL type | Constraints |
|---|---|---|
| `id` | `INT` | Primary key |
| `nfc_id` | `VARCHAR(50)` | Not null |
| `status` | `VARCHAR(20)` | Not null |
| `timestamp` | `DATETIME` | Not null |

This table records dispense, weight, and pickup status strings using the card UID.

## Relationships

```text
medicalStaff 1 ---- * users
users        1 ---- * medical
users        * ---- * pill
              through userPill
users        1 ---- * dispenseLog
pill         1 ---- * dispenseLog
users        1 ---- * dispense_history via NFC UID
```

The dispense-history table is not directly keyed to a patient UUID, so the NFC ID is used to resolve the associated patient when generating dashboard events.

## Initialization

`init_db(app)`:

1. Verifies `SQLALCHEMY_DATABASE_URI`
2. Sets pool pre-ping and recycling options
3. Initializes Flask-SQLAlchemy

`db.create_all()` creates missing SQLAlchemy tables on application startup.

The manual schema file is [pillsExtraction.sql](../database/pillsExtraction.sql).

## Inventory Update

When a dispense status is received:

1. Look up the patient by request UUID or last NFC UID.
2. Find the patient's assigned pill row based on motor index.
3. Decrement `quantity` by the reported dropped pill count.
4. Clamp the quantity to zero or greater.
5. Insert a `DispenseLog`.
6. Insert a `DispenseHistory` record.

If motor 1 is used, the SQLAlchemy code decrements the first `userPill` row sorted by `pillsId`. Motor 2 uses the second row.

## ID Generation

- Staff IDs use `MS` plus six hexadecimal characters.
- User UUIDs use five uppercase hexadecimal characters.
- Medical-history IDs use five uppercase hexadecimal characters.
- Pill IDs use `PIL` plus four uppercase hexadecimal characters.
- These identifiers are random and should be tested against collisions.

## Backups

Example MySQL backup:

```bash
mysqldump -u root -p pillsExtraction > meditrack-backup.sql
```

Example restore:

```bash
mysql -u root -p pillsExtraction < meditrack-backup.sql
```

Backups should include staff credentials and patient data and therefore must be protected and retained according to project policy.
