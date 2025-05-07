from models import Bot, BotCreate
from uuid import UUID, uuid4
import sqlite3
from repos.connections import get_connection


class BotRepository:
    def __init__(self):
        self.conn = get_connection()
        self._ensure_table()

    def _ensure_table(self):
        with self.conn:
            self.conn.execute(
                """
                CREATE TABLE IF NOT EXISTS bots (
                    id TEXT PRIMARY KEY,
                    name TEXT NOT NULL,
                    task TEXT DEFAULT 'Idle'
                )
            """
            )

    def create_bot(self, bot_data: BotCreate) -> Bot:
        bot_id = str(uuid4())
        with self.conn:
            self.conn.execute(
                "INSERT INTO bots (id, name, task) VALUES (?, ?, ?)",
                (bot_id, bot_data.name, bot_data.task),
            )
        return Bot(id=UUID(bot_id), name=bot_data.name, task=bot_data.task)

    def get_all_bots(self):
        cur = self.conn.cursor()
        cur.execute("SELECT id, name, task FROM bots")
        return [
            Bot(id=UUID(row[0]), name=row[1], task=row[2]) for row in cur.fetchall()
        ]

    def get_bot(self, bot_id: UUID):
        cur = self.conn.cursor()
        cur.execute("SELECT id, name, task FROM bots WHERE id = ?", (str(bot_id),))
        row = cur.fetchone()
        if row:
            return Bot(id=UUID(row[0]), name=row[1], task=row[2])
        return None

    def update_task(self, bot_id: UUID, new_task: str):
        with self.conn:
            cur = self.conn.execute(
                "UPDATE bots SET task = ? WHERE id = ?", (new_task, str(bot_id))
            )
        return self.get_bot(bot_id)
