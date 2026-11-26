from database import db

class DispenseLog(db.Model):
    __tablename__ = 'dispenseLog'

    dispenseId = db.Column(db.Integer, primary_key=True)
    UUID = db.Column(db.String(5), db.ForeignKey('users.UUID', ondelete='CASCADE'), nullable=False)
    pillsId = db.Column(db.String(5), db.ForeignKey('pill.pillsId'), nullable=False)
    
    amountDispense = db.Column(db.Integer, nullable=False, default=0)
    status = db.Column(db.Boolean, default=False, nullable=False)
    
    timestamp = db.Column(db.DateTime, nullable=False, default=db.func.current_timestamp())

    def to_dict(self):
        return {
            "dispenseId": self.dispenseId,
            "UUID": self.UUID,
            "pillsId": self.pillsId,
            "amountDispense": self.amountDispense,
            "status": self.status,
            "timestamp": self.timestamp.isoformat(),
        }