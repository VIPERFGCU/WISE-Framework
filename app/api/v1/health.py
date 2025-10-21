from fastapi import APIRouter, Depends
from app import deps

router = APIRouter(prefix="/health", tags=["health"])

# Public Checks

@router.get("", summary="Health check")
async def health():
    return {"status": "ok"}

@router.get("/version", summary="API version")
async def version():
    return {"version": "1.0.0"}

# Secure checks
@router.get("/secure", summary="JWT-protected health", dependencies=[Depends(deps.require_role("viewer"))])
async def secure_health():
    return {"status": "ok", "auth": "jwt"}

@router.get("/secure-key", summary="API key protected health", dependencies=[Depends(deps.require_api_key)])
async def secure_health_api_key():
    return {"status": "ok", "auth": "api_key"}

@router.get("/secure-either", summary="JWT or API key health", dependencies=[Depends(deps.require_api_key_or_role("viewer"))])
async def secure_health_either():
    return {"status": "ok", "auth": "api_key_or_jwt"}
