"""Dashboard routes — aggregated data for the HTML UI."""
from flask import Blueprint
from database import db
from models import User, Medical, UserPill, Pill, OnSchedule
from utils import success, error

dashboard_bp = Blueprint("dashboard", __name__)


@dashboard_bp.route("/dashboard/<uuid>", methods=["GET"])
def get_dashboard(uuid):
    """GET /api/dashboard/<uuid> — everything needed for the dashboard."""
    user = User.query.get(uuid)
    if not user:
        return error("User not found", 404)

    # Medical history + staff name
    medical = Medical.query.filter_by(UUID=uuid).first()
    medical_staff_name = medical.medicalStaffName if medical else "Unknown"

    # User's pill inventory
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
            "weight": float(pill.weight),
            "quantity": inv.quantity,
            "pillsDose": inv.pillsDose,
            "DESCRIPTION": pill.DESCRIPTION,
            "lowStock": inv.quantity < 10,
        }
        for inv, pill in inventory
    ]

    # Schedules with pill names
    schedules = (
        db.session.query(OnSchedule, Pill)
        .join(Pill, Pill.pillsId == OnSchedule.pillsId)
        .filter(OnSchedule.UUID == uuid, OnSchedule.isActive == True)
        .order_by(OnSchedule.timing)
        .all()
    )
    schedule_data = [
        {
            **s.to_dict(),
            "pillName": p.pillName,
        }
        for s, p in schedules
    ]

    return success({
        "user": {
            "name": user.NAME,
            "nfcId": user.NFCId,
            "NRIC": "",                     # add a column if needed
            "email": user.email,
            "phoneNumber": user.phoneNumber,
            "medicalStaffName": medical_staff_name,
        },
        "inventory": inventory_data,
        "schedules": schedule_data,
    })


@dashboard_bp.route("/stats/<uuid>", methods=["GET"])
def get_stats(uuid):
    """GET /api/stats/<uuid> — quick summary statistics for a user."""
    total_pills = (
        db.session.query(db.func.coalesce(db.func.sum(UserPill.quantity), 0))
        .filter_by(UUID=uuid)
        .scalar()
    )
    active_schedules = OnSchedule.query.filter_by(UUID=uuid, isActive=True).count()

    return success({
        "totalPills": int(total_pills),
        "activeSchedules": active_schedules,
    })