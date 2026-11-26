import uuid as uuid_lib
from flask import Blueprint, current_app, request
from models.dispenseLog import DispenseLog
from models.dispenseLog import DispenseLog
from sqlalchemy.exc import IntegrityError

from database import db
from models import MedicalStaff, User, Medical
from models.pill import Pill
from models.userPill import UserPill
from utils import (
    success, error, require_fields,
    hash_password, verify_password, create_token, decode_token, 
    validate_email, validate_nric, validate_phone_number
)

staffauth_bp = Blueprint("staff", __name__)


def _current_staff():
    """Return the currently logged-in staff member, or None."""
    auth = request.headers.get("Authorization", "")
    if not auth.startswith("Bearer "):
        return None
    payload = decode_token(auth.split(" ", 1)[1])
    if not payload or payload.get("role") != "staff":
        return None
    return MedicalStaff.query.get(payload["uuid"])


@staffauth_bp.route("/staff/signup", methods=["POST"])
def staff_signup():
    """POST /api/staff/signup — register a new staff account."""
    data = request.get_json() or {}

    missing = require_fields(data, ["name", "email", "password", "phoneNumber"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    email = data["email"].lower().strip()

    if MedicalStaff.query.filter_by(email=email).first():
        return error("Email already registered", 409)

    for _ in range(10):
        ms_id = "MS" + uuid_lib.uuid4().hex[:3].upper()
        if not MedicalStaff.query.get(ms_id):
            break
    else:
        return error("Could not generate unique staff ID", 500)

    try:
        staff = MedicalStaff(
            MSId=ms_id,
            NAME=data["name"].strip(),
            email=email,
            phoneNumber=data["phoneNumber"].strip(),
            passwordHash=hash_password(data["password"]),
        )
        db.session.add(staff)
        db.session.commit()
    except IntegrityError as e:
        db.session.rollback()
        return error(f"Constraint violation: {str(e.orig)}", 409)
    except Exception as e:
        db.session.rollback()
        return error(f"Database error: {str(e)}", 500)

    token = create_token(ms_id, role="staff")
    return success(
        {"token": token, "staff": staff.to_dict()},
        "Staff account created",
        201,
    )

@staffauth_bp.route("/staff/login", methods=["POST"])
def staff_login():
    """POST /api/staff/login — authenticate a staff member."""
    data = request.get_json() or {}

    missing = require_fields(data, ["email", "password"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    email = data["email"].lower().strip()
    staff = MedicalStaff.query.filter_by(email=email).first()

    if not staff or not staff.passwordHash:
        return error("Invalid email or password", 401)
    if not verify_password(data["password"], staff.passwordHash):
        return error("Invalid email or password", 401)

    token = create_token(staff.MSId, role="staff")
    return success(
        {"token": token, "staff": staff.to_dict()},
        "Login successful",
    )


@staffauth_bp.route("/staff/me", methods=["GET"])
def staff_me():
    """GET /api/staff/me — return the current staff from token."""
    staff = _current_staff()
    if not staff:
        return error("Invalid or missing token", 401)
    return success(staff.to_dict())


@staffauth_bp.route("/staff/dashboard", methods=["GET"])
def staff_dashboard():
    """GET /api/staff/dashboard — aggregated data for the staff view."""
    staff = _current_staff()
    if not staff:
        return error("Invalid or missing token", 401)

    patients = User.query.filter_by(MSId=staff.MSId).all()

    patient_list = []
    for p in patients:
        medical = Medical.query.filter_by(UUID=p.UUID).first()
        patient_list.append({
            "UUID": p.UUID,
            "NAME": p.NAME,
            "email": p.email,
            "phoneNumber": p.phoneNumber,
            "NFCId": p.NFCId,
            "medicalHistoryId": p.medicalHistoryId,
            "medicalStaffName": medical.medicalStaffName if medical else "Unknown",
            "nric": p.nric,
        })

    return success({
        "staff": staff.to_dict(),
        "patientCount": len(patient_list),
        "patients": patient_list,
    })
    
@staffauth_bp.route("/staff/patients", methods=["POST"])
def staff_add_patient():
    """POST /api/staff/patients — staff creates a patient account."""
    staff = _current_staff()
    if not staff:
        return error("Invalid or missing token", 401)

    data = request.get_json() or {}
    
    missing = require_fields(data, ["name", "email", "nric", "NFCid"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)
        
    email = data["email"].strip().lower()
    nric = data["nric"].strip().upper()
    phone = data.get("phoneNumber", "").strip()
    
    if not validate_email(email):
        return error("Invalid Email format.", 400)
        
    if not validate_nric(nric):
        return error("Invalid NRIC. Must be in format S1234567A.", 400)
        
    if phone and not validate_phone_number(phone):
        return error("Invalid Phone Number. Must be a valid 8-digit SG number.", 400)

    if User.query.filter_by(email=email).first():
        return error("Email already registered", 409)
    if User.query.filter_by(NFCId=data["NFCid"]).first():
        return error("NFC wristband already registered", 409)

    user_uuid = None
    for _ in range(10):
        temp_uuid = uuid_lib.uuid4().hex[:5].upper()
        if not User.query.get(temp_uuid):
            user_uuid = temp_uuid
            break
            
    medical_history_id = None
    for _ in range(10):
        temp_id = uuid_lib.uuid4().hex[:5].upper()
        if not Medical.query.get(temp_id):
            medical_history_id = temp_id
            break

    try:
        user = User(
            UUID=user_uuid,
            NAME=data["name"].strip(),
            email=email,                
            nric=nric,                  
            phoneNumber=phone,
            MSId=staff.MSId,
            NFCId=data["NFCid"].strip(),
            medicalHistoryId=medical_history_id,
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

    return success(user.to_dict(), "Patient added", 201)

@staffauth_bp.route("/staff/pills", methods=["GET"])
def staff_list_pills():
    """GET /api/staff/pills — list pills in the catalogue."""
    staff = _current_staff()
    if not staff:
        return error("Invalid or missing token", 401)

    pills = Pill.query.all()
    return success([p.to_dict() for p in pills])

@staffauth_bp.route("/staff/pills", methods=["POST"])
def staff_add_pill():
    """POST /api/staff/pills — add a pill to the catalogue."""
    staff = _current_staff()
    if not staff:
        return error("Invalid or missing token", 401)

    data = request.get_json() or {}
    missing = require_fields(data, ["pillName"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    for _ in range(10):
        pills_id = "PIL" + uuid_lib.uuid4().hex[:2].upper()
        if not Pill.query.get(pills_id):
            break

    try:
        pill = Pill(
            pillsId=pills_id,
            pillName=data["pillName"].strip(),
            DESCRIPTION=data.get("DESCRIPTION", "").strip(),
        )
        db.session.add(pill)
        db.session.commit()
    except Exception as e:
        db.session.rollback()
        return error(f"Database error: {str(e)}", 500)

    return success(pill.to_dict(), "Pill added", 201)

@staffauth_bp.route("/staff/patients/<uuid>/pills", methods=["POST"])
def staff_assign_pill(uuid):
    """POST /api/staff/patients/<uuid>/pills — assign a pill to a patient."""
    staff = _current_staff()
    if not staff:
        return error("Invalid or missing token", 401)

    if not User.query.get(uuid):
        return error("Patient not found", 404)

    data = request.get_json() or {}
    missing = require_fields(data, ["pillsId"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    if not Pill.query.get(data["pillsId"]):
        return error("Pill not found", 404)

    existing = UserPill.query.get((uuid, data["pillsId"]))
    if existing:
        existing.quantity = int(data.get("quantity", existing.quantity))
        existing.pillsDose = int(data.get("pillsDose", existing.pillsDose))
        db.session.commit()
        return success(existing.to_dict(), "Assignment updated")

    try:
        up = UserPill(
            UUID=uuid,
            pillsId=data["pillsId"],
            quantity=int(data.get("quantity", 0)),
            pillsDose=int(data.get("pillsDose", 1)),
        )
        db.session.add(up)
        db.session.commit()
    except Exception as e:
        db.session.rollback()
        return error(f"Database error: {str(e)}", 500)

    return success(up.to_dict(), "Pill assigned", 201)

@staffauth_bp.route("/staff/patients/<uuid>", methods=["GET"])
def staff_get_patient(uuid):
    """GET /api/staff/patients/<uuid> — full patient view."""
    staff = _current_staff()
    if not staff:
        return error("Invalid or missing token", 401)

    patient = User.query.get(uuid)
    if not patient:
        return error("Patient not found", 404)

    inventory = (
        db.session.query(UserPill, Pill)
        .join(Pill, Pill.pillsId == UserPill.pillsId)
        .filter(UserPill.UUID == uuid)
        .all()
    )
    inventory_data = [
        {
            "pillsId": pill.pillsId,
            "pillName": pill.pillName,
            "quantity": inv.quantity,
            "pillsDose": inv.pillsDose,
        }
        for inv, pill in inventory
    ]
    
    dispenser_data = []
    for inv, pill in inventory:
        logs = DispenseLog.query.filter_by(UUID=uuid, pillsId=pill.pillsId).all()
        dispensed = sum((l.amountDispense or 0) for l in logs)
        dispenser_data.append({
            "pillName": pill.pillName,
            "dispensed": dispensed,
            "remaining": inv.quantity,     
        })

    patient_temps = current_app.config.get("PATIENT_TEMPS", {})
    latest_temp = patient_temps.get(uuid, current_app.config.get("LATEST_TEMP"))
    
    return success({
        "patient": patient.to_dict(),
        "inventory": inventory_data,
        "dispenserDetails": dispenser_data,
        "temperature": latest_temp
    })