from fastapi import APIRouter
from app.repos.bots import BotRepository
from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates
from fastapi import Request

router = APIRouter()
repo = BotRepository()
templates = Jinja2Templates(directory="templates")


@router.get("/dashboard", response_class=HTMLResponse)
def bot_dashboard(request: Request):
    bots = repo.get_all_bots()
    return templates.TemplateResponse(
        "status.html",
        {
            "request": request,
            "bots": bots,
        },
    )
