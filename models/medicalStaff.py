"""Medical staff model."""
from database import db


class MedicalStaff(db.Model):
    __tablename__ = "medicalStaff"

    MSId         = db.Column(db.CHAR(5),      primary_key=True)
    NAME         = db.Column(db.String(32),   nullable=False)
    email        = db.Column(db.String(128),  nullable=False, unique=True)
    phoneNumber  = db.Column(db.String(20),   nullable=False)
    passwordHash = db.Column(db.String(255))          # ← NEW

    def to_dict(self, include_password=False):
        data = {
            "MSId": self.MSId,
            "NAME": self.NAME,
            "email": self.email,
            "phoneNumber": self.phoneNumber,
        }
        if include_password:
            data["passwordHash"] = self.passwordHash
        return data

    def __repr__(self):
        return f"<MedicalStaff {self.MSId}: {self.NAME}>"