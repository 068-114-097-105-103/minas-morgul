from fastapi import APIRouter, Request, Form, Depends
from fastapi.responses import RedirectResponse
from fastapi.templating import Jinja2Templates
from app.repos.users import UserRepository
import os

templates = Jinja2Templates(
    directory=os.path.join(os.path.dirname(__file__), "../templates")
)

router = APIRouter()


@router.get("/login")
def login_page(request: Request):
    return templates.TemplateResponse("login.html", {"request": request})


@router.post("/login")
def login(
    request: Request,
    username: str = Form(...),
    password: str = Form(...),
    user_repo: UserRepository = Depends(),
):
    if user_repo.verify_user(username, password):
        request.session["user"] = username
        return RedirectResponse("/info/tasks", status_code=302)
    return templates.TemplateResponse(
        "login.html", {"request": request, "error": "Invalid credentials"}
    )


@router.get("/logout")
def logout(request: Request):
    request.session.clear()
    return RedirectResponse("/login", status_code=302)
