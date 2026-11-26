from database import db


class Pill(db.Model):
    __tablename__ = "pill"

    pillsId     = db.Column(db.CHAR(5), primary_key=True)
    pillName    = db.Column(db.CHAR(32), nullable=False)
    DESCRIPTION = db.Column(db.Text)

    def to_dict(self):
        return {
            "pillsId": self.pillsId,
            "pillName": self.pillName,
            "DESCRIPTION": self.DESCRIPTION,
        }