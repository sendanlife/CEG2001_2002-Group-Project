from database import db


class OnSchedule(db.Model):
    __tablename__ = "onSchedule"

    scheduleId      = db.Column(db.CHAR(5), primary_key=True)
    UUID            = db.Column(db.CHAR(5), db.ForeignKey("users.UUID"), nullable=False)
    pillsId         = db.Column(db.CHAR(5), db.ForeignKey("pill.pillsId"), nullable=False)
    pillsDose       = db.Column(db.Integer, nullable=False, default=1)
    onRepeat        = db.Column(db.String(64))    # stored as "MON,WED,FRI"
    timing          = db.Column(db.Time, nullable=False)
    dispenseAmount  = db.Column(db.Integer, nullable=False, default=1)
    isActive        = db.Column(db.Boolean, nullable=False, default=True)
    isRepeat        = db.Column(db.Boolean, nullable=False, default=True)

    def to_dict(self):
        return {
            "scheduleId": self.scheduleId,
            "UUID": self.UUID,
            "pillsId": self.pillsId,
            "pillsDose": self.pillsDose,
            "onRepeat": self.onRepeat,
            "timing": str(self.timing),
            "dispenseAmount": self.dispenseAmount,
            "isActive": self.isActive,
            "isRepeat": self.isRepeat,
        }