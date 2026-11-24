from .auth import auth_bp        # ✅ the blueprint variable
from .user import users_bp
from .pill import pills_bp
from .schedule import schedule_bp
from .dashboard import dashboard_bp
from .ingest import ingest_bp
from .staffAuth import staffauth_bp

__all__ = [
    "auth_bp",
    "users_bp",
    "pills_bp",
    "schedule_bp",
    "dashboard_bp",
    "ingest_bp",
    "staffauth_bp",
]