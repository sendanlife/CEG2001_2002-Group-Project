"""MediTrack IoT Backend - Main entry point."""
from flask import Flask, send_from_directory
from flask_cors import CORS
from flask_socketio import SocketIO
from config import Config

# Database
from database import init_db, db

# Blueprints
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

    # ─── Initialize MySQL ────────────────────
    init_db(app)

    # ─── Create tables if they don't exist ───
    with app.app_context():
        # Import models so SQLAlchemy knows about them
        import models  # noqa
        db.create_all()

    # ─── Register blueprints ─────────────────
    for bp in (auth_bp, users_bp, pills_bp, schedule_bp, dashboard_bp, ingest_bp, staffauth_bp):
        app.register_blueprint(bp, url_prefix="/api")

    @app.route("/")
    def indexPage():
        """Landing page — choose Staff or Patient."""
        return send_from_directory("static", "index.html")

    @app.route("/dashboard")
    def patientDashboardPage():
        """Patient dashboard (requires login)."""
        return send_from_directory("static", "dashboard.html")
    # Might create a function where patients are able to change their password created by the staff.

    @app.route("/login")
    def loginPage():
        return send_from_directory("static", "login.html")

    @app.route("/signup")
    def signupPage():
        return send_from_directory("static", "signup.html")
    # Might remove signup for patients in the future, as they should be created by staff.
    
    @app.route("/staff/login")
    def staffLoginPage():
        return send_from_directory("static", "staffLogin.html")
    # login email: syafiq@nuh.sg
    # login password: 12345678

    @app.route("/staff/signup")
    def staffSignupPage():
        return send_from_directory("static", "staffSignup.html")

    @app.route("/staff/dashboard")
    def staffDashboardPage():
        return send_from_directory("static", "staffDashboard.html")

    @app.route("/health")
    def health():
        return {"status": "healthy", "service": "meditrack"}

    app.extensions["socketio"] = socketio
    socketio.init_app(app)

    return app


@socketio.on("connect")
def handleConnect():
    print("🔌 Dashboard connected")


@socketio.on("disconnect")
def handleDisconnect():
    print("🔌 Dashboard disconnected")


if __name__ == "__main__":
    app = create_app()
    print(f"🚀 MediTrack backend running on http://localhost:{Config.PORT}")
    socketio.run(app, host="0.0.0.0", port=Config.PORT, debug=Config.DEBUG)