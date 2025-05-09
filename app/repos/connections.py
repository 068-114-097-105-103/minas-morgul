import sqlite3
import os

DB_PATH = os.getenv("DB_PATH", "/app/data/botdb.sqlite3")


def get_connection():
    conn = sqlite3.connect(DB_PATH, check_same_thread=False)
    conn.execute(
        """
        PRAGMA foreign_keys = ON;
        """
    )
    return conn
