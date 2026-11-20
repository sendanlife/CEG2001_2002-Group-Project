"""MediTrack IoT Backend - Main entry point."""
from flask import Flask, send_from_directory
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

    @app.route("/api/temperature", methods=["POST"])
    def receive_temperature():
        data = request.get_json(silent=True)
        if not data or "temperature" not in data:
            return jsonify({"success": False, "message": "Missing temperature"}), 400

        try:
            temp = round(float(data["temperature"]), 1)
        except (TypeError, ValueError):
            return jsonify({"success": False, "message": "Invalid temperature value"}), 400

        is_alert = temp < 15.0 or temp > 25.0

        event_payload = {
            "temperature": temp,
            "eventType": "TEMP_ALERT" if is_alert else "TEMP_UPDATE",
            "timestamp": datetime.datetime.now().isoformat(),
        }

        socketio.emit("new-event", event_payload)

        return jsonify({"success": True, "data": event_payload}), 200

    @app.route("/api/dispense-status", methods=["POST"])
    def receive_dispense_status():
        data = request.get_json(silent=True) or {}
        status = str(data.get("status", "")).upper()
        user_name = app.config.get("LAST_DISPENSE_USER", "Unknown")

        if status == "OK":
            event_payload = {
                "eventType": "PILL_DISPENSED",
                "userId": user_name,
                "timestamp": datetime.datetime.now().isoformat(),
            }
        else:
            event_payload = {
                "eventType": "DISPENSE_ERROR",
                "userId": user_name,
                "timestamp": datetime.datetime.now().isoformat(),
            }

        socketio.emit("new-event", event_payload)

        return jsonify({"success": True, "data": event_payload}), 200

    
    @app.route("/api/verify_nfc", methods=["POST"])
    def verify_nfc():
        data = request.get_json()
        uid = data.get("uid")

        if uid == "4396B815":
            try:
                from models import DispenseHistory
                new_log = DispenseHistory(
                    nfc_id=uid,
                    status="SUCCESS",
                    timestamp=datetime.datetime.now()
                )
                db.session.add(new_log)
                db.session.commit()
            except Exception as e:
                print(f"MySQL Logging Error: {e}")

            socketio.emit("new-event", {
                "eventType": "PILL_DISPENSED",
                "userId": "Syafiq",
                "timestamp": datetime.datetime.now().isoformat()
            })

            return jsonify({"status": "approved"}), 200

        return jsonify({"status": "denied"}), 403

    # @app.route("/")
    # def indexPage():
    #     return send_from_directory("static", "index.html")

    # @app.route("/dashboard")
    # def patientDashboardPage():
    #     """Patient dashboard (requires login)."""
    #     return send_from_directory("static", "dashboard.html")
    # Might create a function where patients are able to change their password created by the staff.

    # @app.route("/login")
    # def loginPage():
    #     return send_from_directory("static", "login.html")

    # @app.route("/signup")
    # def signupPage():
    #     return send_from_directory("static", "signup.html")
    # Might remove signup for patients in the future, as they should be created by staff.
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
            from models import OnSchedule  # ✅ FIXED: Use correct model name
            
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
            from models import OnSchedule  # ✅ FIXED: Use correct model name
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