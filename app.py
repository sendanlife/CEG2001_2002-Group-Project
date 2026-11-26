"""MediTrack IoT Backend - Main entry point."""
import datetime

from flask import Flask, jsonify, send_from_directory, request, current_app
from flask_cors import CORS
from flask_socketio import SocketIO
from config import Config
from database import init_db, db
from routes import (
    users_bp, pills_bp, schedule_bp,
    dashboard_bp, ingest_bp, auth_bp, staffauth_bp
)
from models import User, UserPill, DispenseLog

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

    @app.route("/api/temperature", methods=["POST"])
    def receive_temperature():
        data = request.get_json(silent=True)
        if not data or "temperature" not in data:
            return jsonify({"success": False, "message": "Missing temperature"}), 400

        try:
            temp = round(float(data["temperature"]), 1)
        except (TypeError, ValueError):
            return jsonify({"success": False, "message": "Invalid temperature value"}), 400

        user_uuid = data.get("userId") or app.config.get("ACTIVE_PATIENT_UUID")
        app.config["LATEST_TEMP"] = temp
        if user_uuid:
            app.config.setdefault("PATIENT_TEMPS", {})[user_uuid] = temp

        is_alert = temp < 15.0 or temp > 30.0

        event_payload = {
            "userId": user_uuid,
            "temperature": temp,
            "eventType": "TEMP_ALERT" if is_alert else "TEMP_UPDATE",
            "timestamp": datetime.datetime.now().isoformat(),
        }

        socketio.emit("new-event", event_payload)
        return jsonify({"success": True, "data": event_payload}), 200

    @app.route("/api/dispense-status", methods=["POST"])
    def receive_dispense_status():
        data = request.get_json(silent=True) or {}
        motor    = int(data.get("motor", 0))
        dropped  = int(data.get("dropped", 0))
        expected = int(data.get("expected", 1))
        uuid_in  = str(data.get("uuid", "")).strip()

        if dropped <= 0:
            status = "FAILED"
        elif dropped < expected:
            status = "UNSUCCESSFUL"
        else:
            status = "SUCCESS"
        is_ok = status == "SUCCESS"

        nfc_id = current_app.config.get("LAST_DISPENSE_NFC", "UNKNOWN")
        user = User.query.get(uuid_in) if uuid_in else None
        if not user:
            user = User.query.filter(User.NFCId.ilike(nfc_id)).first()
        user_name = current_app.config.get("LAST_DISPENSE_USER", "Unknown")
        now = datetime.datetime.now()

        print(f"DISPENSE: uuid={uuid_in} nfc={nfc_id} user={'found' if user else 'NOT FOUND'} "
              f"motor={motor} dropped={dropped}/{expected} -> {status}")

        # 1) stock + per-pill log first, in its own commit
        try:
            if user and motor in (1, 2) and dropped > 0:
                ups = UserPill.query.filter_by(UUID=user.UUID).order_by(UserPill.pillsId).all()
                if len(ups) >= motor:
                    up = ups[motor - 1]
                    before = up.quantity or 0
                    up.quantity = max(0, before - dropped)
                    db.session.add(DispenseLog(
                        UUID=user.UUID, pillsId=up.pillsId,
                        amountDispense=dropped, status=is_ok, timestamp=now,
                    ))
                    db.session.commit()
                    print(f"DISPENSE: {up.pillsId} quantity {before} -> {up.quantity}")
                else:
                    print(f"DISPENSE: patient has no pill for motor {motor}")
        except Exception as e:
            db.session.rollback()
            print(f"DispenseLog/quantity error: {e}")

        # 2) history in a separate commit, so a failure here can't undo the stock
        try:
            db.session.add(DispenseHistory(nfc_id=nfc_id, status=status, timestamp=now))
            db.session.commit()
        except Exception as e:
            db.session.rollback()
            print(f"DispenseHistory error: {e}")

        event_payload = {
            "eventType": "PILL_DISPENSED" if is_ok else "DISPENSE_ERROR",
            "status": status,
            "userId": user_name,
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
            weight_g = max(0.0, round(float(data["weight"]), 1))
        except (TypeError, ValueError):
            return jsonify({"success": False, "message": "Invalid weight value"}), 400

        pill_weight_g = 0.5  # Update with your measured single-pill weight in grams
        estimated_pills = int(round(weight_g / pill_weight_g))

        if estimated_pills <= 0:
            stock_state = "EMPTY_MACHINE"
        elif estimated_pills < 5:
            stock_state = "LOW_STOCK"
        else:
            stock_state = "STOCK_OK"

        prev_state = app.config.get("LAST_STOCK_STATE")
        app.config["LAST_STOCK_STATE"] = stock_state

        event_payload = {
            "weight": weight_g,
            "estimatedPills": estimated_pills,
            "eventType": stock_state if stock_state != prev_state else "STOCK_UPDATE",
            "timestamp": datetime.datetime.now().isoformat(),
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
    
    @app.route('/api/staff/schedules/<scheduleId>/toggle', methods=['PATCH'])
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


    @app.route('/api/staff/schedules/<scheduleId>', methods=['PUT'])
    def update_schedule(scheduleId):
        try:
            from models import OnSchedule
            from datetime import datetime
            
            data = request.get_json()
            schedule = OnSchedule.query.get(scheduleId)
            if not schedule:
                return jsonify({"success": False, "message": "Schedule not found"}), 404

            # Update fields
            schedule.pillsId = data.get('pillsId', schedule.pillsId)
            schedule.pillsDose = data.get('pillsDose', schedule.pillsDose)
            
            # Parse time properly
            if 'timing' in data and data['timing']:
                try:
                    schedule.timing = datetime.strptime(data['timing'], "%H:%M").time()
                except ValueError:
                    return jsonify({"success": False, "message": "Invalid time format"}), 400
                    
            schedule.dispenseAmount = data.get('dispenseAmount', schedule.dispenseAmount)
            schedule.onRepeat = data.get('onRepeat', schedule.onRepeat)
            schedule.isRepeat = data.get('isRepeat', schedule.isRepeat)

            db.session.commit()
            return jsonify({"success": True, "message": "Schedule updated"})
        except Exception as e:
            db.session.rollback()
            return jsonify({"success": False, "message": str(e)}), 500
        
    @app.route('/api/staff/patients/<uuid>', methods=['DELETE'])
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