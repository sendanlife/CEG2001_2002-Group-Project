"""IoT event ingest route (ESP32 → DB)."""
from flask import Blueprint, request, current_app
from database import db
from models import Pill, User, OnSchedule
from utils import success, error, require_fields

ingest_bp = Blueprint("ingest", __name__)


def _verify_api_key() -> bool:
    """Check the X-API-Key header sent by the ESP32."""
    return request.headers.get("X-API-Key") == current_app.config["API_KEY"]


@ingest_bp.route("/ingest", methods=["POST"])
def ingest():
    """
    POST /api/ingest — receives sensor events from the ESP32.

    Expected JSON:
    {
      "deviceId": "MED-DEVICE-01",
      "userId":   "USR01",
      "eventType": "PILL_DISPENSED",
      "pillsId":  "PIL01",
      "temperature": 22.5,
      "weight": 5.0
    }
    """
    if not _verify_api_key():
        return error("Unauthorized", 401)

    data = request.get_json() or {}

    missing = require_fields(data, ["deviceId", "eventType"])
    if missing:
        return error(f"Missing: {', '.join(missing)}", 400)

    event_type = data["eventType"]

    # ─── Side effects per event type ───────────────────────
    if event_type == "PILL_DISPENSED" and data.get("pillsId"):
        pill = Pill.query.get(data["pillsId"])
        if pill and pill.quantity > 0:
            pill.quantity -= 1
            db.session.commit()

    elif event_type == "EMPTY_MACHINE" and data.get("pillsId"):
        pill = Pill.query.get(data["pillsId"])
        if pill:
            pill.quantity = 0
            db.session.commit()

    # You can also add dispenseLog entries here if you create that table.

    # ─── Push real-time event to all dashboards ────────────
    socketio = current_app.extensions.get("socketio")
    if socketio:
        socketio.emit("new-event", data)

    return success(data, "Event recorded", 201)