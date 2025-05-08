@echo off
SETLOCAL

:: Step 1: Create virtual environment if missing
if not exist .venv (
    echo Creating virtual environment...
    python -m venv .venv
)

:: Step 2: Activate the virtual environment
echo Activating virtual environment...
call .venv\Scripts\activate.ps1

:: Step 3: Install dependencies
echo Installing dependencies...
pip install --upgrade pip
pip install -r requirements.txt

:: Step 4: Run the FastAPI server
echo Starting FastAPI server...
uvicorn app.main:app --host 10.113.210.251 --port 8888 --reload --proxy-headers --ssl_keyfile "dev.key" --ssl_certfile "dev.crt",

ENDLOCAL
pause
