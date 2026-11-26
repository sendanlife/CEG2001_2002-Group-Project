import datetime
from flask import Blueprint, current_app, request
from database import db
from models import User
from utils import (
    success, error, require_fields, create_token, decode_token, hash_password, verify_password
)

auth_bp = Blueprint("auth", __name__)


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

    scanned_uid = data["uid"].strip().upper()
    print(f"ESP32 scanned hardware card: {scanned_uid}")
    current_app.config["LAST_DISPENSE_NFC"] = scanned_uid
    user = User.query.filter(User.NFCId.ilike(scanned_uid)).first()
    current_app.config["LAST_DISPENSE_USER"] = user.NAME if user else "Unknown"

    if not user:
        return error("Unauthorized card tap. NFC ID not found in database.", 403)

    current_app.config["ACTIVE_PATIENT_UUID"] = user.UUID

    return success({
        "patientName": user.NAME,
        "nfcId": user.NFCId,
        "uuid": user.UUID,
        "status": "approved",
    }, "NFC authentication successful. Dispensing medication.", 200)


@auth_bp.route("/auth/assign-nfc", methods=["POST"])
def assign_nfc():
    data = request.get_json() or {}
    missing = require_fields(data, ["uuid", "nfcId"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    uuid_val = data["uuid"].strip().upper()
    nfc_id = data["nfcId"].strip().upper()

    if not nfc_id:
        return error("NFC ID cannot be empty", 400)

    user = User.query.filter_by(UUID=uuid_val).first()
    if not user:
        return error("User not found", 404)

    existing_owner = User.query.filter(
        User.NFCId.ilike(nfc_id),
        User.UUID != user.UUID
    ).first()
    if existing_owner:
        return error(f"NFC ID already assigned to {existing_owner.NAME} ({existing_owner.UUID})", 409)

    try:
        user.NFCId = nfc_id
        db.session.commit()
    except Exception as e:
        db.session.rollback()
        return error(f"Database error: {str(e)}", 500)

    current_app.config["PENDING_NFC_UID"] = ""

    return success({
        "patientName": user.NAME,
        "nfcId": user.NFCId,
        "uuid": user.UUID,
        "status": "NFC ID assigned successfully",
    }, "NFC ID assigned successfully.", 200)


@auth_bp.route("/auth/pending-nfc", methods=["POST"])
def set_pending_nfc():
    data = request.get_json(silent=True) or {}
    uid = (data.get("uid") or "").strip().upper()
    if not uid:
        return error("Missing uid", 400)

    current_app.config["PENDING_NFC_UID"] = uid

    # Push live autofill to any open Add Patient / Assign NFC modal
    socketio = current_app.extensions.get("socketio")
    if socketio:
        socketio.emit("new-event", {"eventType": "NFC_SCANNED", "uid": uid})

    return success({"nfcId": uid}, "Pending NFC UID saved for autofill", 200)


@auth_bp.route("/auth/pending-nfc", methods=["GET"])
def get_pending_nfc():
    uid = (current_app.config.get("PENDING_NFC_UID") or "").strip().upper()
    return success({"nfcId": uid}, "Fetched pending NFC UID", 200)