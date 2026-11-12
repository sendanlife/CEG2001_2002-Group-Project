"""Schedule routes."""
from flask import Blueprint, request
from datetime import datetime
from database import db
from models import OnSchedule, User, Pill
from utils import success, error, require_fields

schedule_bp = Blueprint("schedule", __name__)


def _parse_time(value: str):
    """Accept 'HH:MM' or 'HH:MM:SS'."""
    for fmt in ("%H:%M:%S", "%H:%M"):
        try:
            return datetime.strptime(value, fmt).time()
        except ValueError:
            continue
    return None


@schedule_bp.route("/schedule", methods=["POST"])
def create_schedule():
    """POST /api/schedule — create a medication schedule."""
    data = request.get_json() or {}

    missing = require_fields(data, ["scheduleId", "UUID", "pillsId", "timing"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    if OnSchedule.query.get(data["scheduleId"]):
        return error("Schedule ID already exists", 409)
    if not User.query.get(data["UUID"]):
        return error("User not found", 404)
    if not Pill.query.get(data["pillsId"]):
        return error("Pill not found", 404)

    parsed_time = _parse_time(data["timing"])
    if not parsed_time:
        return error("Invalid time format. Use HH:MM or HH:MM:SS", 400)

    schedule = OnSchedule(
        scheduleId=data["scheduleId"],
        UUID=data["UUID"],
        pillsId=data["pillsId"],
        pillsDose=data.get("pillsDose", 1),
        onRepeat=data.get("onRepeat"),          # e.g. "MON,WED,FRI" or None
        timing=parsed_time,
        dispenseAmount=data.get("dispenseAmount", 1),
        isActive=data.get("isActive", True),
        isRepeat=data.get("isRepeat", True),
    )
    db.session.add(schedule)
    db.session.commit()
    return success(schedule.to_dict(), "Schedule created", 201)


@schedule_bp.route("/schedule", methods=["GET"])
def list_schedules():
    """GET /api/schedule?UUID=<uuid> — list all schedules (optionally filtered)."""
    uuid = request.args.get("UUID")
    query = OnSchedule.query
    if uuid:
        query = query.filter_by(UUID=uuid)
    schedules = query.order_by(OnSchedule.timing).all()
    return success([s.to_dict() for s in schedules])


@schedule_bp.route("/schedule/<schedule_id>", methods=["GET"])
def get_schedule(schedule_id):
    """GET /api/schedule/<schedule_id>."""
    schedule = OnSchedule.query.get(schedule_id)
    if not schedule:
        return error("Schedule not found", 404)
    return success(schedule.to_dict())


@schedule_bp.route("/schedule/<schedule_id>", methods=["PUT"])
def update_schedule(schedule_id):
    """PUT /api/schedule/<schedule_id> — update timing, repeat days, etc."""
    schedule = OnSchedule.query.get(schedule_id)
    if not schedule:
        return error("Schedule not found", 404)

    data = request.get_json() or {}
    if "timing" in data:
        parsed = _parse_time(data["timing"])
        if not parsed:
            return error("Invalid time format", 400)
        schedule.timing = parsed
    if "onRepeat" in data:
        schedule.onRepeat = data["onRepeat"]
    if "pillsDose" in data:
        schedule.pillsDose = data["pillsDose"]
    if "dispenseAmount" in data:
        schedule.dispenseAmount = data["dispenseAmount"]
    if "isActive" in data:
        schedule.isActive = data["isActive"]
    if "isRepeat" in data:
        schedule.isRepeat = data["isRepeat"]

    db.session.commit()
    return success(schedule.to_dict(), "Schedule updated")


@schedule_bp.route("/schedule/<schedule_id>", methods=["DELETE"])
def delete_schedule(schedule_id):
    """DELETE /api/schedule/<schedule_id>."""
    schedule = OnSchedule.query.get(schedule_id)
    if not schedule:
        return error("Schedule not found", 404)

    db.session.delete(schedule)
    db.session.commit()
    return success(None, "Schedule deleted")


@schedule_bp.route("/schedule/next/<uuid>", methods=["GET"])
def next_dose(uuid):
    """GET /api/schedule/next/<uuid> — the next dose due for this user."""
    now_time = datetime.utcnow().time()
    schedule = (
        OnSchedule.query
        .filter_by(UUID=uuid, isActive=True)
        .filter(OnSchedule.timing >= now_time)
        .order_by(OnSchedule.timing)
        .first()
    )
    if not schedule:
        return success(None, "No upcoming doses today")
    return success(schedule.to_dict())