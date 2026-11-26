from .auth import auth_bp     
from .user import users_bp
from .pill import pills_bp
from .staffAuth import staffauth_bp

__all__ = [
    "auth_bp",
    "users_bp",
    "pills_bp",
    "staffauth_bp",
]