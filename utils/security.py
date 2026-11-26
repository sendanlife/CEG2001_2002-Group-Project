from datetime import datetime, timedelta
from typing import Optional
from werkzeug.security import generate_password_hash, check_password_hash
import jwt

from config import Config

def hash_password(password: str) -> str:
    if not password:
        raise ValueError("Password cannot be empty")
    return generate_password_hash(password, method="pbkdf2:sha256")


def verify_password(password: str, hashed: str) -> bool:
    if not password or not hashed:
        return False
    try:
        return check_password_hash(hashed, password)
    except Exception:
        return False

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