"""Input validation helpers."""


def require_fields(data: dict, fields: list) -> list:
    """Return a list of missing required fields."""
    if not data:
        return list(fields)
    return [field for field in fields if field not in data or data[field] in (None, "")]