import datetime
import uuid as uuid_lib
from sqlalchemy.exc import IntegrityError
from flask import Blueprint, current_app, request
from database import db
from models import User, Medical, MedicalStaff
from utils import (
    success, error, require_fields,
    hash_password, verify_password, create_token, decode_token,
)

auth_bp = Blueprint("auth", __name__)


@auth_bp.route("/auth/signup", methods=["POST"])
def signup():
    data = request.get_json() or {}

    missing = require_fields(data, ["name", "email", "password", "NFCid"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    email = data["email"].lower().strip()

    if User.query.filter_by(email=email).first():
        return error("Email already registered", 409)
    if User.query.filter_by(NFCId=data["NFCid"]).first():
        return error("NFC wristband already registered", 409)

    staff = MedicalStaff.query.get("MS000")
    if not staff:
        return error(
            "System staff 'MS000' not found. Run the SQL to create it.",
            500,
        )

    user_uuid = uuid_lib.uuid4().hex[:5].upper()
    medical_history_id = uuid_lib.uuid4().hex[:5].upper()

    try:
        user = User(
            UUID=user_uuid,
            NAME=data["name"].strip(),
            email=email,
            nric=data.get("NRIC"),
            phoneNumber=data.get("phoneNumber", "").strip(),
            MSId=staff.MSId,
            NFCId=data["NFCid"].strip(),
            medicalHistoryId=medical_history_id,
            passwordHash=hash_password(data["password"]),
        )
        db.session.add(user)
        db.session.flush()

        medical = Medical(
            medicalHistoryId=medical_history_id,
            UUID=user_uuid,
            issueBy=staff.MSId,
            medicalStaffName=staff.NAME,
        )
        db.session.add(medical)
        db.session.commit()

    except IntegrityError as e:
        db.session.rollback()
        return error(f"Constraint violation: {str(e.orig)}", 409)
    except Exception as e:
        db.session.rollback()
        return error(f"Database error: {str(e)}", 500)

    token = create_token(user_uuid)
    return success(
        {"token": token, "user": user.to_dict()},
        "Account created",
        201,
    )


@auth_bp.route("/auth/login", methods=["POST"])
def login():
    data = request.get_json() or {}

    missing = require_fields(data, ["email", "password"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    user = User.query.filter_by(email=data["email"].lower()).first()
    if not user or not user.passwordHash or not verify_password(data["password"], user.passwordHash):
        return error("Invalid email or password", 401)

    token = create_token(user.UUID)
    return success(
        {"token": token, "user": user.to_dict()},
        "Login successful",
    )


@auth_bp.route("/auth/me", methods=["GET"])
def me():
    auth_header = request.headers.get("Authorization", "")
    if not auth_header.startswith("Bearer "):
        return error("Missing token", 401)

    payload = decode_token(auth_header.split(" ", 1)[1])
    if not payload:
        return error("Invalid or expired token", 401)

    user = User.query.get(payload["uuid"])
    if not user:
        return error("User not found", 404)

    return success(user.to_dict())


@auth_bp.route("/auth/verify-nfc", methods=["POST"])
def verify_nfc():
    data = request.get_json() or {}

    missing = require_fields(data, ["uid"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    scanned_uid = data["uid"].strip()
    print(f"ESP32 scanned hardware card: {scanned_uid}")

    user = User.query.filter_by(NFCId=scanned_uid).first()

    if user:
        print(f"Card Authorized. Patient: {user.NAME}. Sending approval to ESP32.")
        current_app.config["LAST_DISPENSE_USER"] = user.NAME

        return success(
            {
                "patientName": user.NAME,
                "nfcId": user.NFCId,
                "status": "approved"
            },
            "NFC authentication successful. Dispensing medication.",
            200
        )
    else:
        print(f"Card Denied. No user found with NFC ID: {scanned_uid}")
        return error("Unauthorized card tap. NFC ID not found in database.", 403)