from app.models import Bot, BotCreate, Task
from uuid import UUID, uuid4
from app.repos.connections import get_connection
from app.repos.tasks import TaskRepository


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
                    name TEXT NOT NULL
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

    def add_task(self, bot_id: UUID, new_task: Task):
        task_repo = TaskRepository()
        task = task_repo.create_task(new_task)
        with self.conn:
            self.conn.execute(
                "UPDATE bots SET task = ? WHERE id = ?",
                (str(task.id), str(bot_id)),
            )
        return task

    def update_task(self, bot_id: UUID, new_task: Task):
        task_repo = TaskRepository()
        task = task_repo.update_task(new_task.id, new_task)
        with self.conn:
            self.conn.execute(
                "UPDATE bots SET task = ? WHERE id = ?",
                (str(task.id), str(bot_id)),
            )
        return task

    def delete_bot(self, bot_id: UUID):
        with self.conn:
            self.conn.execute("DELETE FROM bots WHERE id = ?", (str(bot_id),))
        return True
