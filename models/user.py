from datetime import datetime
from database import db


class User(db.Model):
    __tablename__ = "users"

    UUID              = db.Column(db.CHAR(5), primary_key=True)
    NAME              = db.Column(db.String(32), nullable=False)
    email             = db.Column(db.String(128), nullable=False, unique=True)
    phoneNumber       = db.Column(db.String(20), nullable=False)
    MSId              = db.Column(db.CHAR(5), db.ForeignKey("medicalStaff.MSId"))
    NFCId             = db.Column(db.CHAR(20), unique=True)
    medicalHistoryId  = db.Column(db.CHAR(5))
    passwordHash      = db.Column(db.String(255))   # NEW: for login

    def to_dict(self, include_password=False):
        data = {
            "UUID": self.UUID,
            "NAME": self.NAME,
            "email": self.email,
            "phoneNumber": self.phoneNumber,
            "MSId": self.MSId,
            "NFCId": self.NFCId,
            "medicalHistoryId": self.medicalHistoryId,
        }
        if include_password:
            data["passwordHash"] = self.passwordHash
        return data