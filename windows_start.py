import uvicorn
import os

cmd = "python -m venv ./.venv && ./.venv/Scripts/activate && pip install -r requirements.txt"
os.system(cmd)

if __name__ == "__main__":
    uvicorn.run(
        "app.main:app",
        port=8888,
        ssl_keyfile="dev.key",
        ssl_certfile="dev.crt",
    )
