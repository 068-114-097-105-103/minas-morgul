from fastapi import FastAPI
from app.routes.commands import router as commands_router
from app.routes.info import router as info_router

app = FastAPI()
app.include_router(commands_router, prefix="/telemetry")
app.include_router(info_router, prefix="/info")
