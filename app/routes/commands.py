from fastapi import APIRouter, HTTPException
from app.models import Bot, BotCreate
from typing import List
from uuid import UUID

router = APIRouter()

# In-memory DB (don’t use this in prod, obviously)
fake_db: List[Bot] = []


@router.post("/bots/", response_model=Bot)
def create_bot(bot_data: BotCreate):
    bot = Bot(**bot_data.model_dump())
    fake_db.append(bot)
    return bot


@router.get("/bots/", response_model=List[Bot])
def get_all_bots():
    return fake_db


@router.get("/bots/{bot_id}", response_model=Bot)
def get_bot(bot_id: UUID):
    for bot in fake_db:
        if bot.id == bot_id:
            return bot
    raise HTTPException(status_code=404, detail="Bot not found")


@router.put("/bots/{bot_id}", response_model=Bot)
def update_task(bot_id: UUID, task: str):
    for bot in fake_db:
        if bot.id == bot_id:
            bot.task = task
            return bot
    raise HTTPException(status_code=404, detail="Bot not found")
