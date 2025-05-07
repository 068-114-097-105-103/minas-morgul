from fastapi import APIRouter
from app.repos.bots import BotRepository
from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates
from fastapi import Request, Depends
from app.repos.tasks import TaskRepository
import os

router = APIRouter()
templates = Jinja2Templates(
    directory=os.path.join(os.path.dirname(__file__), "../templates")
)


@router.get("/dashboard", response_class=HTMLResponse)
def bot_dashboard(request: Request, repo: BotRepository = Depends()):
    bots = repo.get_all_bots()
    return templates.TemplateResponse(
        "status.html",
        {
            "request": request,
            "bots": bots,
        },
    )


@router.get("/tasks", response_class=HTMLResponse)
def task_dashboard(
    request: Request,
    bot_repo: BotRepository = Depends(),
    task_repo: TaskRepository = Depends(),
):
    tasks = task_repo.get_all_tasks()
    bots = bot_repo.get_all_bots()
    if not bots:
        return HTMLResponse(
            content="No bots registered. Please register a bot first.", status_code=200
        )
    return templates.TemplateResponse(
        "command.html",
        {
            "request": request,
            "tasks": tasks,
            "bots": bots,
        },
    )
