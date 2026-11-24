"""Input validation helpers."""
import re

def require_fields(data: dict, fields: list) -> list:
    """Return a list of missing required fields."""
    if not isinstance(data, dict):
        return list(fields)
    return [field for field in fields if field not in data or data[field] is None or str(data[field]).strip() == ""]

def validate_email(email: str) -> bool:
    """Basic email format validation."""
    if not email: return False
    pattern = r"^[\w\.-]+@[\w\.-]+\.\w+$"
    return bool(re.match(pattern, str(email).strip()))

def validate_nric(nric: str) -> bool:
    """Strict Singapore NRIC validation."""
    if not nric: return False
    # Force uppercase just in case
    clean_nric = str(nric).strip().upper()
    # MUST start with S, T, F, or G. Followed by 7 digits. Ends with 1 letter.
    pattern = r"^[STFG]\d{7}[A-Z]$"
    return bool(re.match(pattern, clean_nric))

def validate_phone_number(phone: str) -> bool:
    """Strict phone number validation."""
    if not phone: return False
    
    clean_phone = re.sub(r"[\s\-\(\)]", "", str(phone))
    
    sg_pattern = r"^\+65[89]\d{7}$"
    
    local_pattern = r"^[89]\d{7}$"
    
    return bool(re.match(sg_pattern, clean_phone) or re.match(local_pattern, clean_phone))