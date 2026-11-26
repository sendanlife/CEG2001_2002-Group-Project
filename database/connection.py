"""MySQL connection handler using SQLAlchemy."""
from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy()


def init_db(app):

    if not app.config.get("SQLALCHEMY_DATABASE_URI"):
        raise RuntimeError(
            "SQLALCHEMY_DATABASE_URI is not set. "
            "Check your config.py / .env file."
        )

    app.config.setdefault("SQLALCHEMY_TRACK_MODIFICATIONS", False)
    app.config.setdefault("SQLALCHEMY_ENGINE_OPTIONS", {
        "pool_pre_ping": True, 
        "pool_recycle": 280,    
        "pool_size": 10,
        "max_overflow": 20,
    })

    db.init_app(app)
    return db


def get_db():
    return db

class DispenseHistory(db.Model):
    __tablename__ = 'dispense_history'
    id = db.Column(db.Integer, primary_key=True)
    nfc_id = db.Column(db.String(50), nullable=False)
    status = db.Column(db.String(20), nullable=False)
    timestamp = db.Column(db.DateTime, nullable=False)