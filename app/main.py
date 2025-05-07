from fastapi import FastAPI
from app.routes.commands import router as commands_router

app = FastAPI()
app.include_router(commands_router, prefix="/commands")
