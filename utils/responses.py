"""Standardized JSON responses."""
from flask import jsonify


def success(data=None, message: str = "OK", status: int = 200):
    """Return a standardized success response."""
    return jsonify({
        "success": True,
        "message": message,
        "data": data
    }), status


def error(message: str, status: int = 400, details=None):
    """Return a standardized error response."""
    return jsonify({
        "success": False,
        "message": message,
        "details": details
    }), status