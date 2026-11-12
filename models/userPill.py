from database import db


class UserPill(db.Model):
    __tablename__ = "userPill"

    UUID      = db.Column(db.CHAR(5), db.ForeignKey("users.UUID"), primary_key=True)
    pillsId   = db.Column(db.CHAR(5), db.ForeignKey("pill.pillsId"), primary_key=True)
    quantity  = db.Column(db.Integer, nullable=False, default=0)
    pillsDose = db.Column(db.Integer, nullable=False, default=1)

    def to_dict(self):
        return {
            "UUID": self.UUID,
            "pillsId": self.pillsId,
            "quantity": self.quantity,
            "pillsDose": self.pillsDose,
        }