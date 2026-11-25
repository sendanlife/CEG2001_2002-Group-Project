"""MediTrack IoT Backend - Main entry point."""
import datetime
from flask import Flask, jsonify, send_from_directory, request
from flask_cors import CORS
from flask_socketio import SocketIO
from config import Config
from database import init_db, db
from routes import (
    users_bp, pills_bp, schedule_bp,
    dashboard_bp, ingest_bp, auth_bp, staffauth_bp
)

socketio = SocketIO(cors_allowed_origins="*")


def create_app() -> Flask:
    app = Flask(__name__, static_folder="static")
    app.config.from_object(Config)
    app.config["SQLALCHEMY_DATABASE_URI"] = Config.SQLALCHEMY_DATABASE_URI
    app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False

    CORS(app, resources={r"/api/*": {"origins": "*"}})

    init_db(app)

    with app.app_context():
        import models
        from database.connection import DispenseHistory
        db.create_all()

    for bp in (auth_bp, users_bp, pills_bp, schedule_bp, dashboard_bp, ingest_bp, staffauth_bp):
        app.register_blueprint(bp, url_prefix="/api")

    hardware_state = {
        "temperature": None,
        "tempAlert": False,
        "cupStatus": "IDLE",
        "pillSize": None,
        "lastWeightG": None,
        "lastPatient": None,
        "lastUpdated": None,
        "recentEvents": [],
    }

    def _record_hw_event(label, detail, severity="ok"):
        hardware_state["recentEvents"].insert(0, {
            "label": label,
            "detail": detail,
            "severity": severity,
            "timestamp": datetime.datetime.now().isoformat(),
        })
        hardware_state["recentEvents"] = hardware_state["recentEvents"][:10]

    def _resolve_last_patient():
        """Resolve patient UUID & NAME from the last NFC tap so dashboard rows match."""
        nfc_id = app.config.get("LAST_DISPENSE_NFC", "UNKNOWN")
        fallback_user = app.config.get("LAST_DISPENSE_USER", "Unknown")
        user_uuid = None
        user_name = fallback_user

        try:
            from models import User
            user = None
            if nfc_id and nfc_id != "UNKNOWN":
                user = User.query.filter_by(NFCId=nfc_id).first()
            if user is None and fallback_user and fallback_user != "Unknown":
                user = User.query.get(fallback_user)
            if user is not None:
                user_uuid = user.UUID
                user_name = user.NAME
        except Exception as exc:
            print(f"Patient lookup skipped: {exc}")

        return user_uuid, user_name, nfc_id

    @app.route("/api/hardware/latest", methods=["GET"])
    def get_latest_hardware():
        return jsonify({"success": True, "data": hardware_state}), 200

    @app.route("/api/temperature", methods=["POST"])
    def receive_temperature():
        data = request.get_json(silent=True)
        if not data or "temperature" not in data:
            return jsonify({"success": False, "message": "Missing temperature"}), 400

        try:
            temp = round(float(data["temperature"]), 1)
        except (TypeError, ValueError):
            return jsonify({"success": False, "message": "Invalid temperature value"}), 400

        is_alert = temp < 15.0 or temp > 30.0
        now = datetime.datetime.now().isoformat()

        hardware_state["temperature"] = temp
        hardware_state["tempAlert"] = is_alert
        hardware_state["lastUpdated"] = now

        event_payload = {
            "temperature": temp,
            "eventType": "TEMP_ALERT" if is_alert else "TEMP_UPDATE",
            "timestamp": now,
        }

        socketio.emit("new-event", event_payload)
        return jsonify({"success": True, "data": event_payload}), 200

    @app.route("/api/dispense-status", methods=["POST"])
    def receive_dispense_status():
        data = request.get_json(silent=True) or {}
        status = str(data.get("status", "")).upper()
        pill_size = str(data.get("pillSize", "SMALL")).strip().upper()
        is_ok = status == "OK"

        user_uuid, user_name, nfc_id = _resolve_last_patient()
        now = datetime.datetime.now()

        try:
            new_log = DispenseHistory(
                nfc_id=nfc_id,
                status=f"{pill_size}_IR_OK" if is_ok else f"{pill_size}_IR_FAIL",
                timestamp=now,
            )
            db.session.add(new_log)
            db.session.commit()
        except Exception as e:
            db.session.rollback()
            print(f"MySQL Logging Error: {e}")

        hardware_state["pillSize"] = pill_size
        hardware_state["lastPatient"] = user_name
        hardware_state["lastUpdated"] = now.isoformat()
        hardware_state["cupStatus"] = "DISPENSED_IR_OK" if is_ok else "DISPENSE_ERROR"
        _record_hw_event(
            f"{pill_size} IR {'OK' if is_ok else 'FAIL'}",
            f"Patient: {user_name}",
            "ok" if is_ok else "error",
        )

        event_payload = {
            "eventType": "PILL_DISPENSED" if is_ok else "DISPENSE_ERROR",
            "pillSize": pill_size,
            "status": status,
            "userId": user_uuid,
            "patientName": user_name,
            "timestamp": now.isoformat(),
        }

        socketio.emit("new-event", event_payload)
        return jsonify({"success": True, "data": event_payload}), 200

    @app.route("/api/weight", methods=["POST"])
    def receive_weight():
        data = request.get_json(silent=True)
        if not data or "weight" not in data:
            return jsonify({"success": False, "message": "Missing weight"}), 400

        try:
            weight_added_g = max(0.0, round(float(data["weight"]), 2))
        except (TypeError, ValueError):
            return jsonify({"success": False, "message": "Invalid weight value"}), 400

        PILL_PROFILES = {
            "SMALL": {"weight_g": 0.50},   # Update with your measured small pill weight
            "LARGE": {"weight_g": 1.20},   # Update with your measured large pill weight
        }
        pill_size = str(data.get("pillSize", "SMALL")).strip().upper()
        pill_weight_g = PILL_PROFILES.get(pill_size, PILL_PROFILES["SMALL"])["weight_g"]
        min_detect_g = round(pill_weight_g * 0.5, 2)

        verified = weight_added_g >= min_detect_g
        dispensed_pills = max(1, int(round(weight_added_g / pill_weight_g))) if verified else 0

        user_uuid, user_name, nfc_id = _resolve_last_patient()
        now = datetime.datetime.now()

        if not verified:
            try:
                db.session.add(DispenseHistory(nfc_id=nfc_id, status=f"{pill_size}_WEIGHT_FAIL", timestamp=now))
                db.session.commit()
            except Exception as e:
                db.session.rollback()
                print(f"MySQL Weight Fail Logging Error: {e}")

        event_type = "DISPENSE_VERIFIED" if verified else "DISPENSE_WEIGHT_FAIL"
        hardware_state["pillSize"] = pill_size
        hardware_state["lastWeightG"] = weight_added_g
        hardware_state["lastPatient"] = user_name
        hardware_state["lastUpdated"] = now.isoformat()
        hardware_state["cupStatus"] = "WAITING_PICKUP" if verified else "WEIGHT_FAIL"
        _record_hw_event(
            f"{pill_size} Weight {'Verified' if verified else 'Failed'}",
            f"+{weight_added_g:.2f} g ({dispensed_pills} pill) · {user_name}",
            "ok" if verified else "error",
        )

        event_payload = {
            "eventType": event_type,
            "pillSize": pill_size,
            "weightAdded": weight_added_g,
            "dispensedPills": dispensed_pills,
            "verified": verified,
            "userId": user_uuid,
            "patientName": user_name,
            "timestamp": now.isoformat(),
        }

        socketio.emit("new-event", event_payload)
        return jsonify({"success": True, "data": event_payload}), 200

    @app.route("/api/pickup-status", methods=["POST"])
    def receive_pickup_status():
        data = request.get_json(silent=True)
        if not data or "status" not in data:
            return jsonify({"success": False, "message": "Missing status"}), 400

        raw_status = str(data["status"]).strip().upper()
        pill_size = str(data.get("pillSize", "SMALL")).strip().upper()

        if raw_status == "TAKEN":
            event_type = "PILL_TAKEN"
            db_status = f"{pill_size}_TAKEN"
        elif raw_status == "TIMEOUT":
            event_type = "PILL_MISSED"
            db_status = f"{pill_size}_MISSED"
        else:
            return jsonify({"success": False, "message": "Invalid pickup status"}), 400

        user_uuid, user_name, nfc_id = _resolve_last_patient()
        now = datetime.datetime.now()

        try:
            db.session.add(DispenseHistory(nfc_id=nfc_id, status=db_status, timestamp=now))
            db.session.commit()
        except Exception as e:
            db.session.rollback()
            print(f"MySQL Pickup Logging Error: {e}")

        hardware_state["pillSize"] = pill_size
        hardware_state["lastPatient"] = user_name
        hardware_state["lastUpdated"] = now.isoformat()
        hardware_state["cupStatus"] = "TAKEN" if raw_status == "TAKEN" else "MISSED"
        _record_hw_event(
            f"{pill_size} Pill {'Taken' if raw_status == 'TAKEN' else 'Missed'}",
            f"Patient: {user_name}",
            "ok" if raw_status == "TAKEN" else "warning",
        )

        event_payload = {
            "eventType": event_type,
            "pillSize": pill_size,
            "status": raw_status,
            "userId": user_uuid,
            "patientName": user_name,
            "timestamp": now.isoformat(),
        }

        socketio.emit("new-event", event_payload)
        return jsonify({"success": True, "data": event_payload}), 200

    @app.route("/")
    def staffStartPage():
        return send_from_directory("static", "staffLogin.html")

    @app.route("/staff/login")
    def staffLoginPage():
        return send_from_directory("static", "staffLogin.html")

    @app.route("/staff/signup")
    def staffSignupPage():
        return send_from_directory("static", "staffSignup.html")

    @app.route("/staff/dashboard")
    def staffDashboardPage():
        return send_from_directory("static", "staffDashboard.html")

    @app.route("/health")
    def health():
        return {"status": "healthy", "service": "meditrack"}

    @app.route("/api/staff/schedules/<scheduleId>/toggle", methods=["PATCH"])
    def toggle_schedule(scheduleId):
        try:
            from models import OnSchedule

            schedule = OnSchedule.query.get(scheduleId)
            if not schedule:
                return jsonify({"success": False, "message": "Schedule not found"}), 404

            schedule.isActive = not schedule.isActive
            db.session.commit()

            return jsonify({"success": True, "message": "Status toggled"})
        except Exception as e:
            db.session.rollback()
            return jsonify({"success": False, "message": str(e)}), 500

    @app.route("/api/staff/schedules/<scheduleId>", methods=["PUT"])
    def update_schedule(scheduleId):
        try:
            from models import OnSchedule

            data = request.get_json()
            schedule = OnSchedule.query.get(scheduleId)
            if not schedule:
                return jsonify({"success": False, "message": "Schedule not found"}), 404

            schedule.pillsId = data.get("pillsId", schedule.pillsId)
            schedule.pillsDose = data.get("pillsDose", schedule.pillsDose)

            if "timing" in data and data["timing"]:
                try:
                    schedule.timing = datetime.datetime.strptime(data["timing"], "%H:%M").time()
                except ValueError:
                    return jsonify({"success": False, "message": "Invalid time format"}), 400

            schedule.dispenseAmount = data.get("dispenseAmount", schedule.dispenseAmount)
            schedule.onRepeat = data.get("onRepeat", schedule.onRepeat)
            schedule.isRepeat = data.get("isRepeat", schedule.isRepeat)

            db.session.commit()
            return jsonify({"success": True, "message": "Schedule updated"})
        except Exception as e:
            db.session.rollback()
            return jsonify({"success": False, "message": str(e)}), 500

    @app.route("/api/staff/patients/<uuid>", methods=["DELETE"])
    def delete_patient(uuid):
        try:
            from models import User

            patient = User.query.get(uuid)
            if not patient:
                return jsonify({"success": False, "message": "Patient not found"}), 404

            db.session.delete(patient)
            db.session.commit()
            return jsonify({"success": True, "message": "Patient deleted"})
        except Exception as e:
            db.session.rollback()
            return jsonify({"success": False, "message": str(e)}), 500

    app.extensions["socketio"] = socketio
    socketio.init_app(app)

    return app


@socketio.on("connect")
def handleConnect():
    print("Dashboard connected")


@socketio.on("disconnect")
def handleDisconnect():
    print("Dashboard disconnected")


if __name__ == "__main__":
    app = create_app()
    print(f"MediTrack backend running on http://localhost:{Config.PORT}")
    socketio.run(app, host="0.0.0.0", port=Config.PORT, debug=Config.DEBUG)