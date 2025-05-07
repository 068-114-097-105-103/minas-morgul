from fastapi import APIRouter, HTTPException, Depends
from app.models import Bot, BotCreate
from typing import List
from uuid import UUID
from app.repos.bots import BotRepository

router = APIRouter()


@router.post("/bots/", response_model=Bot)
def create_bot(bot_data: BotCreate, repo: BotRepository = Depends()):
    bot = Bot(**bot_data.model_dump())
    repo.create_bot(bot)
    return bot


@router.get("/bots/", response_model=List[Bot])
def get_all_bots(repo: BotRepository = Depends()):
    return repo.get_all_bots()


@router.get("/bots/{bot_id}", response_model=Bot)
def get_bot(bot_id: UUID, repo: BotRepository = Depends()):
    bot = repo.get_bot(bot_id)
    if bot:
        return bot
    else:
        raise HTTPException(status_code=404, detail="Bot not found")


@router.put("/bots/{bot_id}", response_model=Bot)
def update_task(bot_id: UUID, task: str, repo: BotRepository = Depends()):
    update = repo.update_task(bot_id, task)
    if update:
        return update
    else:
        raise HTTPException(status_code=404, detail="Bot not found")
