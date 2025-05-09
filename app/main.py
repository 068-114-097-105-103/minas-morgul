from fastapi import FastAPI, Request
from fastapi.responses import RedirectResponse
from app.routes.commands import router as commands_router
from app.routes.info import router as info_router
from app.routes.auth import router as auth_router
from starlette.middleware.sessions import SessionMiddleware
import os


app = FastAPI()

app.add_middleware(
    SessionMiddleware,
    secret_key=os.environ.get("SECRET_KEY", "supersecretkey"),  # Change in prod!
)


app.include_router(commands_router, prefix="/telemetry")
app.include_router(info_router, prefix="/info")
app.include_router(auth_router, prefix="/auth")


@app.get("/")
def root(request: Request):
    if request.session.get("user"):
        return RedirectResponse("/info/tasks")
    return RedirectResponse("/auth/login")
