from database import db


class Medical(db.Model):
    __tablename__ = "medical"

    medicalHistoryId  = db.Column(db.CHAR(5), primary_key=True)
    UUID              = db.Column(db.CHAR(5), db.ForeignKey("users.UUID"), nullable=False)
    issueBy           = db.Column(db.CHAR(5), db.ForeignKey("medicalStaff.MSId"), nullable=False)
    medicalStaffName  = db.Column(db.String(32), nullable=False)

    def to_dict(self):
        return {
            "medicalHistoryId": self.medicalHistoryId,
            "UUID": self.UUID,
            "issueBy": self.issueBy,
            "medicalStaffName": self.medicalStaffName,
        }