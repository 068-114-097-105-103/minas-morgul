from app.models import Task, TaskCreate
from uuid import UUID, uuid4
from app.repos.connections import get_connection


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
                    command TEXT DEFAULT 'Idle',
                    parameters TEXT DEFAULT NULL
                )
            """
            )

    def create_task(self, task_data: TaskCreate) -> Task:
        task_id = str(uuid4())
        with self.conn:
            self.conn.execute(
                "INSERT INTO tasks (id, command, parameters) VALUES (?, ?, ?)",
                (task_id, task_data.command, task_data.parameters),
            )
        return Task(
            id=UUID(task_id), command=task_data.command, parameters=task_data.parameters
        )

    def update_task(self, task_id: UUID, new_task: Task) -> Task:
        with self.conn:
            self.conn.execute(
                "UPDATE tasks SET command = ?, parameters = ? WHERE id = ?",
                (new_task.command, new_task.parameters, str(task_id)),
            )
        return self.get_task(task_id)

    def get_task(self, task_id: UUID) -> Task:
        cur = self.conn.cursor()
        cur.execute(
            "SELECT id, command, parameters FROM tasks WHERE id = ?", (str(task_id),)
        )
        row = cur.fetchone()
        if row:
            return Task(id=UUID(row[0]), command=row[1], parameters=row[2])
        return None

    def get_all_tasks(self):
        cur = self.conn.cursor()
        cur.execute("SELECT id, command, parameters FROM tasks")
        return [
            Task(id=UUID(row[0]), command=row[1], parameters=row[2])
            for row in cur.fetchall()
        ]

    def delete_task(self, task_id: UUID):
        with self.conn:
            self.conn.execute("DELETE FROM tasks WHERE id = ?", (str(task_id),))
        return True
