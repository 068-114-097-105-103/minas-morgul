from pydantic import BaseModel, Field
from typing import Optional
from uuid import UUID, uuid4


class BotBase(BaseModel):
    name: str
    task: Optional[str] = "Idle"


class BotCreate(BotBase):
    pass


class Bot(BotBase):
    id: UUID = Field(default_factory=uuid4)
