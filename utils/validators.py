import re

def require_fields(data: dict, fields: list) -> list:
    if not isinstance(data, dict):
        return list(fields)
    return [field for field in fields if field not in data or data[field] is None or str(data[field]).strip() == ""]

def validate_email(email: str) -> bool:
    if not email: return False
    pattern = r"^[\w\.-]+@[\w\.-]+\.\w+$"
    return bool(re.match(pattern, str(email).strip()))

def validate_nric(nric: str) -> bool:
    if not nric: return False
    clean_nric = str(nric).strip().upper()
    pattern = r"^[STFG]\d{7}[A-Z]$"
    return bool(re.match(pattern, clean_nric))

def validate_phone_number(phone: str) -> bool:
    if not phone: return False
    
    clean_phone = re.sub(r"[\s\-\(\)]", "", str(phone))
    
    sg_pattern = r"^\+65[89]\d{7}$"
    
    local_pattern = r"^[89]\d{7}$"
    
    return bool(re.match(sg_pattern, clean_phone) or re.match(local_pattern, clean_phone))