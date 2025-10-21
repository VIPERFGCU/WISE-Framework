from typing import List, Optional, Union
from pydantic import Field, AliasChoices, field_validator
from pydantic_settings import BaseSettings, SettingsConfigDict
import json

class Settings(BaseSettings):
    # API key
    api_key: str = Field(default="devkey", validation_alias=AliasChoices("API_KEY"))

    # JWT
    jwt_secret: str = Field(default="change-me", validation_alias=AliasChoices("JWT_SECRET", "SECRET_KEY"))
    jwt_alg: str = Field(default="HS256", validation_alias=AliasChoices("JWT_ALG"))
    access_token_expire_minutes: int = Field(
        default=30,
        validation_alias=AliasChoices("ACCESS_TOKEN_EXPIRE_MINUTES", "TOKEN_EXPIRE_MINUTES"),
    )
    jwt_issuer: Optional[str] = Field(default=None, validation_alias=AliasChoices("JWT_ISSUER"))
    jwt_audience: Optional[str] = Field(default=None, validation_alias=AliasChoices("JWT_AUDIENCE"))

    # Logging
    api_log_level: str = Field(default="info", validation_alias=AliasChoices("API_LOG_LEVEL", "LOG_LEVEL"))

    # CORS — accept CSV string or JSON array from env
    cors_allow_origins: Union[str, List[str]] = Field(
        default=["*"],
        validation_alias=AliasChoices("CORS_ALLOW_ORIGINS", "CORS_ORIGINS"),
    )

    # Influx
    influx_url: str = Field(default="http://localhost:8086", validation_alias=AliasChoices("INFLUX_URL"))
    influx_token: str = Field(default="changeme", validation_alias=AliasChoices("INFLUX_TOKEN"))
    influx_org: str = Field(default="myorg", validation_alias=AliasChoices("INFLUX_ORG"))
    influx_bucket: str = Field(default="mybucket", validation_alias=AliasChoices("INFLUX_BUCKET"))

    model_config = SettingsConfigDict(
        env_file=".env",
        env_file_encoding="utf-8",
        extra="ignore",
    )

    @field_validator("cors_allow_origins", mode="before")
    @classmethod
    def parse_cors_origins(cls, v: Union[str, List[str], None]):
        if v is None:
            return ["*"]
        if isinstance(v, list):
            return [str(x).strip() for x in v if str(x).strip()]
        if isinstance(v, str):
            s = v.strip()
            if not s:
                return ["*"]
            # Try JSON array first
            try:
                parsed = json.loads(s)
                if isinstance(parsed, list):
                    return [str(x).strip() for x in parsed if str(x).strip()]
            except Exception:
                pass
            # Fallback to CSV
            return [part.strip() for part in s.split(",") if part.strip()]
        # Anything else → default to ["*"]
        return ["*"]

settings = Settings()

