from fastapi import APIRouter, HTTPException, Depends
from app.models import Bot, BotCreate, Task
from app.repos.bots import BotRepository
from app.repos.tasks import TaskRepository
from typing import List
from uuid import UUID

router = APIRouter()


@router.post("/telemetry/", response_model=Bot)
def create_bot(bot_data: BotCreate, repo: BotRepository = Depends()):
    bot = Bot(**bot_data.model_dump())
    repo.create_bot(bot)
    return bot


@router.get("/telemetry/", response_model=List[Bot])
def get_all_bots(repo: BotRepository = Depends()):
    bots = repo.get_all_bots()
    return bots


@router.get("/telemetry/{bot_id}", response_model=Bot)
def get_bot(bot_id: UUID, repo: BotRepository = Depends()):
    bot = repo.get_bot(bot_id)
    if bot:
        return bot
    raise HTTPException(status_code=404, detail="Bot not found")


@router.post("/telemetry/{bot_id}/tasks/", response_model=Task)
def create_task(
    bot_id: UUID,
    task_data: Task,
    bot_repo: BotRepository = Depends(),
    task_repo: TaskRepository = Depends(),
):
    bot = bot_repo.get_bot(bot_id)
    if not bot:
        raise HTTPException(status_code=404, detail="Bot not found")
    task = task_repo.create_task(task_data)
    bot.task = task
    bot_repo.update_task(bot_id, task)
    return task


@router.put("/telemetry/{bot_id}", response_model=Bot)
def update_task(
    bot_id: UUID,
    task: Task,
    bot_repo: BotRepository = Depends(),
    task_repo: TaskRepository = Depends(),
):
    bot = bot_repo.get_bot(bot_id)
    if not bot:
        raise HTTPException(status_code=404, detail="Bot not found")
    task = task_repo.update_task(task.id, task)
    bot_repo.update_task(bot_id, task)
    return bot


@router.post("/telemetry/{bot_id}/tasking/", response_model=Task)
def get_new_tasking(bot_id: UUID, bot_repo: BotRepository = Depends()):
    bot = bot_repo.get_bot(bot_id)
    if not bot:
        raise HTTPException(status_code=404, detail="Bot not found")
    task = bot.task
    if not task:
        raise HTTPException(status_code=404, detail="Task not found")
    return task
