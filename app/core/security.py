# app/core/security.py
from datetime import datetime, timedelta, timezone
from typing import Any, Optional, Callable

from fastapi import Depends, HTTPException, status
from fastapi.security import APIKeyHeader, OAuth2PasswordBearer
from jose import jwt, JWTError
from passlib.context import CryptContext

from app.core.config import settings

# Password hashing
pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")

def get_password_hash(password: str) -> str:
    return pwd_context.hash(password)

def verify_password(plain_password: str, hashed_password: str) -> bool:
    return pwd_context.verify(plain_password, hashed_password)

# JWT helpers
def _now_utc() -> datetime:
    return datetime.now(timezone.utc)

def create_access_token(subject: str, roles: list[str], expires_minutes: Optional[int] = None) -> str:
    exp = _now_utc() + timedelta(minutes=expires_minutes or settings.access_token_expire_minutes)
    payload: dict[str, Any] = {"sub": subject, "roles": roles, "exp": exp}
    if settings.jwt_issuer:
        payload["iss"] = settings.jwt_issuer
    if settings.jwt_audience:
        payload["aud"] = settings.jwt_audience
    return jwt.encode(payload, settings.jwt_secret, algorithm=settings.jwt_alg)

def decode_token(token: str) -> Optional[dict[str, Any]]:  # <-- fixed dict
    try:
        options = {"verify_aud": bool(settings.jwt_audience)}
        decoded = jwt.decode(
            token,
            settings.jwt_secret,
            algorithms=[settings.jwt_alg],
            audience=settings.jwt_audience if settings.jwt_audience else None,
            issuer=settings.jwt_issuer if settings.jwt_issuer else None,
            options=options,
        )
        return decoded
    except JWTError:
        return None

# OAuth2 Bearer for Swagger "Authorize"
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="/api/v1/auth/login")

def get_current_user(token: str = Depends(oauth2_scheme)) -> dict:
    data = decode_token(token)
    if not data:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid or expired token")
    if "sub" not in data or "roles" not in data:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Malformed token")
    return data

def require_role(role: str) -> Callable:
    def _checker(user: dict = Depends(get_current_user)) -> dict:
        roles = user.get("roles", [])
        if role not in roles:
            raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail=f"Missing role: {role}")
        return user
    return _checker

# API key (x-api-key)
api_key_header = APIKeyHeader(name="x-api-key", auto_error=False)

async def require_api_key(key: str | None = Depends(api_key_header)) -> None:
    if key != settings.api_key:
        raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid API key")

# Hybrid: allow API key OR JWT with a role
def require_api_key_or_role(role: Optional[str] = None):
    async def _either(
        key: str | None = Depends(api_key_header),
    ):
        # If API key matches, allow
        if key == settings.api_key:
            return {"auth": "api_key"}

        # Else try JWT via oauth2_scheme
        try:
            bearer = await oauth2_scheme(None)  # type: ignore[arg-type]
        except Exception:
            bearer = None

        if not bearer:
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Missing credentials")

        data = decode_token(bearer)
        if not data:
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail="Invalid or expired token")

        if role and role not in data.get("roles", []):
            raise HTTPException(status_code=status.HTTP_403_FORBIDDEN, detail=f"Missing role: {role}")

        return {"auth": "jwt", "user": data.get("sub"), "roles": data.get("roles", [])}

    return _either

