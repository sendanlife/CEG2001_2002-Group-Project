from database import db


class Pill(db.Model):
    __tablename__ = "pill"

    pillsId     = db.Column(db.CHAR(5), primary_key=True)
    pillName    = db.Column(db.CHAR(32), nullable=False)
    weight      = db.Column(db.Numeric(6, 2), nullable=False)
    quantity    = db.Column(db.Integer, nullable=False, default=0)
    DESCRIPTION = db.Column(db.Text)

    def to_dict(self):
        return {
            "pillsId": self.pillsId,
            "pillName": self.pillName,
            "weight": float(self.weight),
            "quantity": self.quantity,
            "DESCRIPTION": self.DESCRIPTION,
        }