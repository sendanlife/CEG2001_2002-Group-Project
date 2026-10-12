"""Pill catalogue routes."""
from flask import Blueprint, request
from database import db
from models import Pill
from utils import success, error, require_fields

pills_bp = Blueprint("pills", __name__)


@pills_bp.route("/pills", methods=["GET"])
def list_pills():
    """GET /api/pills — list all pill types."""
    pills = Pill.query.all()
    return success([p.to_dict() for p in pills])


@pills_bp.route("/pills/<pills_id>", methods=["GET"])
def get_pill(pills_id):
    """GET /api/pills/<pills_id> — fetch one pill."""
    pill = Pill.query.get(pills_id)
    if not pill:
        return error("Pill not found", 404)
    return success(pill.to_dict())


@pills_bp.route("/pills", methods=["POST"])
def create_pill():
    """POST /api/pills — add a new pill to the catalogue."""
    data = request.get_json() or {}

    missing = require_fields(data, ["pillsId", "pillName", "weight"])
    if missing:
        return error(f"Missing fields: {', '.join(missing)}", 400)

    if Pill.query.get(data["pillsId"]):
        return error("Pill ID already exists", 409)

    pill = Pill(
        pillsId=data["pillsId"],
        pillName=data["pillName"],
        weight=data["weight"],
        quantity=data.get("quantity", 0),
        DESCRIPTION=data.get("DESCRIPTION", ""),
    )
    db.session.add(pill)
    db.session.commit()

    return success(pill.to_dict(), "Pill added", 201)


@pills_bp.route("/pills/<pills_id>", methods=["PUT"])
def update_pill(pills_id):
    """PUT /api/pills/<pills_id> — update name, weight, quantity."""
    pill = Pill.query.get(pills_id)
    if not pill:
        return error("Pill not found", 404)

    data = request.get_json() or {}
    if "pillName" in data:
        pill.pillName = data["pillName"]
    if "weight" in data:
        pill.weight = data["weight"]
    if "quantity" in data:
        pill.quantity = data["quantity"]
    if "DESCRIPTION" in data:
        pill.DESCRIPTION = data["DESCRIPTION"]

    db.session.commit()
    return success(pill.to_dict(), "Pill updated")


@pills_bp.route("/pills/<pills_id>", methods=["DELETE"])
def delete_pill(pills_id):
    """DELETE /api/pills/<pills_id> — cascades to userPill & onSchedule."""
    pill = Pill.query.get(pills_id)
    if not pill:
        return error("Pill not found", 404)

    db.session.delete(pill)
    db.session.commit()
    return success(None, "Pill deleted")


# ─── USER INVENTORY ───────────────────────────────────────────

@pills_bp.route("/users/<uuid>/pills", methods=["GET"])
def list_user_pills(uuid):
    """GET /api/users/<uuid>/pills — pills this user has in inventory."""
    from models import UserPill
    items = UserPill.query.filter_by(UUID=uuid).all()
    return success([i.to_dict() for i in items])


@pills_bp.route("/users/<uuid>/pills", methods=["POST"])
def add_user_pill(uuid):
    """POST /api/users/<uuid>/pills — assign a pill to a user."""
    from models import UserPill, User

    if not User.query.get(uuid):
        return error("User not found", 404)

    data = request.get_json() or {}
    missing = require_fields(data, ["pillsId"])
    if missing:
        return error(f"Missing: {', '.join(missing)}", 400)

    if not Pill.query.get(data["pillsId"]):
        return error("Pill not found", 404)

    existing = UserPill.query.get((uuid, data["pillsId"]))
    if existing:
        return error("Pill already assigned to user", 409)

    item = UserPill(
        UUID=uuid,
        pillsId=data["pillsId"],
        quantity=data.get("quantity", 0),
        pillsDose=data.get("pillsDose", 1),
    )
    db.session.add(item)
    db.session.commit()
    return success(item.to_dict(), "Pill assigned to user", 201)