import sqlite3
import os

DB_PATH = os.getenv("DB_PATH", "botdb.sqlite3")


def get_connection():
    return sqlite3.connect(DB_PATH, check_same_thread=False)
