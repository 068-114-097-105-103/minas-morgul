from pydantic import BaseModel, Field
from typing import Optional, Literal
from uuid import UUID, uuid4


class TaskBase(BaseModel):
    command: Optional[str] = None
    parameters: Optional[str] = None
    status: Optional[Literal["Idle", "Pending", "Sent"]] = "Idle"


class Task(TaskBase):
    id: UUID = Field(default_factory=uuid4)
    time_sent: Optional[str] = None


class TaskCreate(TaskBase):
    id: UUID = Field(default_factory=uuid4)


class BotBase(BaseModel):
    name: Optional[str] = None
    task: Optional[Task] = {
        "command": "Idle",
        "parameters": None,
    }


class BotCreate(BotBase):
    id: UUID = Field()


class Bot(BotBase):
    id: UUID = Field()


class Telemetry(BaseModel):
    uuid: UUID = Field()
    memeory: Optional[str] = None
    cpu: Optional[str] = None
    disk: Optional[str] = None
