"""Central configuration for the MediTrack backend."""
import os
from dotenv import load_dotenv

load_dotenv()


class Config:
    # ─── MySQL ───────────────────────────────────
    DB_HOST = os.getenv("DB_HOST", "127.0.0.1")
    DB_PORT = os.getenv("DB_PORT", "3306")
    DB_USER = os.getenv("DB_USER", "root")
    DB_PASSWORD = os.getenv("DB_PASSWORD", "Bcps2012")
    DB_NAME = os.getenv("DB_NAME", "pillsExtraction")

    # ✅ Class attribute, no method
    SQLALCHEMY_DATABASE_URI = (
        f"mysql+pymysql://{DB_USER}:{DB_PASSWORD}"
        f"@{DB_HOST}:{DB_PORT}/{DB_NAME}"
    )
    SQLALCHEMY_TRACK_MODIFICATIONS = False

    # ─── Server ──────────────────────────────────
    PORT = int(os.getenv("PORT", 3000))
    SECRET_KEY = os.getenv("SECRET_KEY", "dev-secret-key")

    # ─── Security ────────────────────────────────
    API_KEY = os.getenv("API_KEY", "dev-api-key")

    # ─── Environment ─────────────────────────────
    ENV = os.getenv("FLASK_ENV", "development")
    DEBUG = ENV == "development"