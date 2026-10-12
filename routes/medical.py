from flask import Blueprint, request
from models import Pill
from database import db
from utils import success, error, require_fields

pills_bp = Blueprint("medical", __name__)


@pills_bp.route("/medical", methods=["GET"])
def list_pills():
    pills = Pill.query.all()
    return success([p.to_dict() for p in pills])


@pills_bp.route("/medical", methods=["POST"])
def create_pill():
    data = request.get_json()
    missing = require_fields(data, ["pillsId", "pillName", "weight"])
    if missing:
        return error(f"Missing: {', '.join(missing)}", 400)

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