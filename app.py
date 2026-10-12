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
    def dashboard():
        return send_from_directory("static", "dashboard.html")

    @app.route("/login")
    def login_page():
        return send_from_directory("static", "login.html")

    @app.route("/signup")
    def signup_page():
        return send_from_directory("static", "signup.html")
    
    @app.route("/staff/login")
    def staff_login_page():
        return send_from_directory("static", "staffLogin.html")

    @app.route("/staff/signup")
    def staff_signup_page():
        return send_from_directory("static", "staffSignup.html")

    @app.route("/staff/dashboard")
    def staff_dashboard_page():
        return send_from_directory("static", "staffDashboard.html")

    @app.route("/health")
    def health():
        return {"status": "healthy", "service": "meditrack"}

    app.extensions["socketio"] = socketio
    socketio.init_app(app)

    return app


@socketio.on("connect")
def handle_connect():
    print("🔌 Dashboard connected")


@socketio.on("disconnect")
def handle_disconnect():
    print("🔌 Dashboard disconnected")


if __name__ == "__main__":
    app = create_app()
    print(f"🚀 MediTrack backend running on http://localhost:{Config.PORT}")
    socketio.run(app, host="0.0.0.0", port=Config.PORT, debug=Config.DEBUG)