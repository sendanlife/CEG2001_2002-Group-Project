"""User management routes."""
from flask import Blueprint, request
from database import db
from models import User, Medical
from utils import success, error, require_fields

users_bp = Blueprint("user", __name__)


@users_bp.route("/user", methods=["GET"])
def list_users():
    """GET /api/user — list all users."""
    users = User.query.all()
    return success([u.to_dict() for u in users])


@users_bp.route("/user/<uuid>", methods=["GET"])
def get_user(uuid):
    """GET /api/user/<uuid> — fetch one user."""
    user = User.query.get(uuid)
    if not user:
        return error("User not found", 404)
    return success(user.to_dict())


@users_bp.route("/user/nfc/<nfc_id>", methods=["GET"])
def get_user_by_nfc(nfc_id):
    """GET /api/user/nfc/<nfc_id> — used when ESP32 scans a wristband."""
    user = User.query.filter_by(NFCId=nfc_id).first()
    if not user:
        return error("User not found", 404)
    return success(user.to_dict())


@users_bp.route("/user", methods=["POST"])
def create_user():
    """POST /api/user — admin-only user creation (no password)."""
    data = request.get_json() or {}

    missing = require_fields(data, [
        "UUID", "NAME", "email", "NFCid", "medicalHistoryId",
    ])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    if User.query.get(data["UUID"]):
        return error("UUID already exists", 409)
    if User.query.filter_by(email=data["email"].lower()).first():
        return error("Email already registered", 409)

    user = User(
        UUID=data["UUID"],
        NAME=data["NAME"],
        email=data["email"].lower(),
        phoneNumber=data.get("phoneNumber", ""),
        MSId=data.get("MSId"),
        NFCId=data["NFCid"],
        medicalHistoryId=data["medicalHistoryId"],
    )
    db.session.add(user)
    db.session.commit()

    return success(user.to_dict(), "User created", 201)


@users_bp.route("/user/<uuid>", methods=["DELETE"])
def delete_user(uuid):
    """DELETE /api/user/<uuid> — cascades to medical, userPill, onSchedule."""
    user = User.query.get(uuid)
    if not user:
        return error("User not found", 404)

    db.session.delete(user)
    db.session.commit()
    return success(None, "User deleted")