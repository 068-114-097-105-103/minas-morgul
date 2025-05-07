from pydantic import BaseModel, Field
from typing import Optional
from uuid import UUID, uuid4


class TaskBase(BaseModel):
    command: Optional[str] = "Idle"
    parameters: Optional[str] = None


class Task(TaskBase):
    id: UUID = Field(default_factory=uuid4)


class TaskCreate(TaskBase):
    pass


class BotBase(BaseModel):
    name: Optional[str] = None
    task: Optional[Task] = {
        "command": "Idle",
        "parameters": None,
    }


class BotCreate(BotBase):
    id: UUID = Field(default_factory=uuid4)


class Bot(BotBase):
    id: UUID = Field(default_factory=uuid4)


class Telemetry(BaseModel):
    id: UUID = Field(default_factory=uuid4)
    memeory: Optional[str] = None
    cpu: Optional[str] = None
    disk: Optional[str] = None
