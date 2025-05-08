from app.models import Task, TaskCreate
from uuid import UUID, uuid4
from app.repos.connections import get_connection
from typing import Optional


class TaskRepository:
    def __init__(self):
        self.conn = get_connection()
        self._ensure_table()

    def _ensure_table(self):
        with self.conn:
            self.conn.execute(
                """
                CREATE TABLE IF NOT EXISTS tasks (
                    id TEXT PRIMARY KEY,
                    command TEXT DEFAULT NULL,
                    parameters TEXT DEFAULT NULL,
                    status TEXT,
                    time_sent TEXT DEFAULT NULL
                )
            """
            )

    def create_task(self, task_data: TaskCreate) -> Task:
        task_id = str(uuid4())
        with self.conn:
            self.conn.execute(
                "INSERT INTO tasks (id, command, parameters, status) VALUES (?, ?, ?, ?)",
                (
                    task_id,
                    task_data.command,
                    task_data.parameters,
                    task_data.status,
                ),
            )
        return Task(
            id=UUID(task_id),
            command=task_data.command,
            parameters=task_data.parameters,
            status=task_data.status,
        )

    def update_task(self, task_id: UUID, update: Task) -> Task:
        print(f"Updating task {task_id} with data: {update.status}")
        with self.conn:
            self.conn.execute(
                "UPDATE tasks SET command = ?, parameters = ?, status = ? WHERE id = ?",
                (update.command, update.parameters, str(update.status), str(task_id)),
            )
            # Update time_sent if status is "Sent"
            if update.status == "Sent":
                self.conn.execute(
                    "UPDATE tasks SET time_sent = datetime('now') WHERE id = ?",
                    (str(task_id),),
                )
        return self.get_task(task_id)

    def get_task(self, task_id: UUID) -> Task:
        cur = self.conn.cursor()
        cur.execute(
            "SELECT id, command, parameters, status FROM tasks WHERE id = ?",
            (str(task_id),),
        )
        row = cur.fetchone()
        if row:
            return Task(
                id=UUID(row[0]),
                command=row[1],
                parameters=row[2],
                status=row[3],
            )
        return None

    def get_all_tasks(self):
        cur = self.conn.cursor()
        cur.execute("SELECT id, command, parameters, status, time_sent FROM tasks")
        return [
            Task(
                id=UUID(row[0]),
                command=row[1],
                parameters=row[2],
                status=row[3],
                time_sent=row[4],
            )
            for row in cur.fetchall()
        ]

    def delete_task(self, task_id: UUID):
        with self.conn:
            self.conn.execute("DELETE FROM tasks WHERE id = ?", (str(task_id),))
        return True
