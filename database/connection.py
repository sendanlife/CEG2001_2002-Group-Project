"""MySQL connection handler using SQLAlchemy."""
from flask_sqlalchemy import SQLAlchemy


# Global SQLAlchemy instance — used by all models via `from database import db`
db = SQLAlchemy()


def init_db(app):
    """
    Initialize SQLAlchemy with the Flask app.

    Call this exactly once during app startup, BEFORE models are used
    or db.create_all() runs.
    """
    # ── Ensure the required config keys exist ─────────────────
    if not app.config.get("SQLALCHEMY_DATABASE_URI"):
        raise RuntimeError(
            "SQLALCHEMY_DATABASE_URI is not set. "
            "Check your config.py / .env file."
        )

    # ── Configure SQLAlchemy behaviour ────────────────────────
    app.config.setdefault("SQLALCHEMY_TRACK_MODIFICATIONS", False)
    app.config.setdefault("SQLALCHEMY_ENGINE_OPTIONS", {
        "pool_pre_ping": True,   # auto-reconnect if MySQL drops connection
        "pool_recycle": 280,     # recycle connections every 4.5 min
        "pool_size": 10,
        "max_overflow": 20,
    })

    # ── Bind the db instance to this app ──────────────────────
    db.init_app(app)
    return db


def get_db():
    """Return the SQLAlchemy db instance."""
    return db