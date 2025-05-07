from pydantic import BaseModel, Field
from typing import Optional
from uuid import UUID, uuid4


class TaskBase(BaseModel):
    command: str
    parameters: Optional[str] = None


class Task(TaskBase):
    id: UUID = Field(default_factory=uuid4)


class TaskCreate(TaskBase):
    pass


class BotBase(BaseModel):
    name: str
    task: Optional[Task] = {
        "command": "Idle",
        "parameters": None,
    }


class BotCreate(BotBase):
    id: UUID = Field(default_factory=uuid4)


class Bot(BotBase):
    id: UUID = Field(default_factory=uuid4)
