"""Password hashing and JWT token utilities (Python 3.7 compatible)."""
from datetime import datetime, timedelta
from typing import Optional
from werkzeug.security import generate_password_hash, check_password_hash
import jwt

from config import Config


# ─── Password Hashing ───────────────────────────────────────

def hash_password(password: str) -> str:
    """Hash a plaintext password using PBKDF2-SHA256."""
    if not password:
        raise ValueError("Password cannot be empty")
    return generate_password_hash(password, method="pbkdf2:sha256")


def verify_password(password: str, hashed: str) -> bool:
    """Verify a plaintext password against its stored hash."""
    if not password or not hashed:
        return False
    try:
        return check_password_hash(hashed, password)
    except Exception:
        return False


# ─── JWT Tokens ─────────────────────────────────────────────

ALGORITHM = "HS256"
DEFAULT_EXPIRY_HOURS = 24


def create_token(uuid: str, role: str = "patient", expires_hours: int = 24) -> str:
    """Create a JWT token for a user."""
    now = datetime.utcnow()
    payload = {
        "uuid": uuid,
        "role": role,  
        "iat": now,
        "exp": now + timedelta(hours=expires_hours),
    }
    return jwt.encode(payload, Config.SECRET_KEY, algorithm=ALGORITHM)


def decode_token(token: str) -> Optional[dict]:
    """Decode and verify a JWT token. Returns None if invalid/expired."""
    if not token:
        return None
    try:
        return jwt.decode(token, Config.SECRET_KEY, algorithms=[ALGORITHM])
    except jwt.ExpiredSignatureError:
        return None
    except jwt.InvalidTokenError:
        return None
    except Exception:
        return None