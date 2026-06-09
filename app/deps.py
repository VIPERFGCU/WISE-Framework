from functools import lru_cache
from typing import Optional

from influxdb_client import InfluxDBClient
from fastapi import Depends, HTTPException, status

from app.core.config import settings
from app.core import security


def _normalize_role_name(role: str) -> str:
    val = role.strip().lower()
    aliases = {
        "view": "viewer",
    }
    return aliases.get(val, val)


def _normalized_roles(roles: list[str]) -> set[str]:
    return {_normalize_role_name(r) for r in roles}


def _has_required_role(user_roles: list[str], required_role: str) -> bool:
    normalized_user_roles = _normalized_roles(user_roles)
    req = _normalize_role_name(required_role)
    # Admin users implicitly satisfy lower-privilege checks such as viewer.
    return req in normalized_user_roles or "admin" in normalized_user_roles

# Influx client dependency
@lru_cache(maxsize=1)
def get_influx_client() -> Optional[InfluxDBClient]:
    try:
        # Use a short timeout (2 seconds) to fail fast if InfluxDB is unavailable
        return InfluxDBClient(
                url=settings.influx_url,
                token=settings.influx_token,
                org=settings.influx_org,
                timeout=2_000,  # 2 second timeout instead of 10
                )
    except Exception as e:
        # If connection fails, return None (InfluxDB unavailable)
        return None

def get_influx_write_api(client: Optional[InfluxDBClient] = Depends(get_influx_client)):
    if client is None:
        raise HTTPException(status_code=status.HTTP_503_SERVICE_UNAVAILABLE,
                            detail="InfluxDB client not available")
    return client.write_api()

def get_influx_query_api(client: Optional[InfluxDBClient] = Depends(get_influx_client)):
    if client is None:
        return None
    return client.query_api()

# Security dependencies (wrap security.py helpers)
def get_current_user(token: str = Depends(security.oauth2_scheme)) -> dict:
    """Decode JWT and return user payload."""
    data = security.decode_token(token)
    if not data:
        raise HTTPException(
                status_code=status.HTTP_401_UNAUTHORIZED,
                detail="Invalid or expired token",
        )
    return data

def require_role(role: str):
    """Dependency factory to enforce a specific role from JWT."""
    def _checker(user: dict = Depends(get_current_user)):
        roles = user.get("roles", [])
        if not _has_required_role(roles, role):
            raise HTTPException(
                    status_code=status.HTTP_403_FORBIDDEN,
                    detail=f"Missing role: {role}",
            )
        return user
    return _checker

# deps.py
def require_any_role(*roles: str):
    """Allow access if the user has ANY of the listed roles."""
    def _checker(user: dict = Depends(get_current_user)):
        user_roles = user.get("roles", [])
        normalized_required = {_normalize_role_name(r) for r in roles}
        if not any(_has_required_role(user_roles, r) for r in normalized_required):
            raise HTTPException(
                status_code=status.HTTP_403_FORBIDDEN,
                detail=f"Missing any of roles: {', '.join(roles)}",
            )
        return user
    return _checker

# For API key only (forwarder to security.py)
require_api_key = security.require_api_key

# For hybrid auth (API key or role)
require_api_key_or_role = security.require_api_key_or_role
