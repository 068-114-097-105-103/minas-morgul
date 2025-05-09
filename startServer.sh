#!/bin/bash

echo "Upgrading pip"

python3 -m pip install --upgrade pip

echo "Insalling dependancies"

pip install -r requirements.txt

echo "Pip up to date, dependencies installed ready to start"

uvicorn app.main:app --host 0.0.0.0 --reload --port 8888
