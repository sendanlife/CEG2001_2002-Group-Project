from flask import jsonify


def success(data=None, message: str = "OK", status: int = 200):
    return jsonify({
        "success": True,
        "message": message,
        "data": data
    }), status


def error(message: str, status: int = 400, details=None):
    return jsonify({
        "success": False,
        "message": message,
        "details": details
    }), status