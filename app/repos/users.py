from app.models import User, UserCreate
from app.repos.connections import get_connection
from passlib.hash import bcrypt
from typing import Optional


class UserRepository:
    def __init__(self):
        self.conn = get_connection()
        self._ensure_table()

    def _ensure_table(self):
        with self.conn:
            self.conn.execute(
                """
                CREATE TABLE IF NOT EXISTS users (
                    username TEXT PRIMARY KEY,
                    hashed_password TEXT NOT NULL
                )
                """
            )

    def create_user(self, user_data: UserCreate) -> User:
        hashed_pw = bcrypt.hash(user_data.password)
        with self.conn:
            self.conn.execute(
                "INSERT INTO users (username, hashed_password) VALUES (?, ?)",
                (user_data.username, hashed_pw),
            )
        return User(username=user_data.username, hashed_password=hashed_pw)

    def get_user(self, username: str) -> Optional[User]:
        cur = self.conn.cursor()
        cur.execute(
            "SELECT username, hashed_password FROM users WHERE username = ?",
            (username,),
        )
        row = cur.fetchone()
        if row:
            return User(username=row[0], hashed_password=row[1])
        return None

    def verify_user(self, username: str, password: str) -> bool:
        user = self.get_user(username)
        if not user:
            return False
        return bcrypt.verify(password, user.hashed_password)


repo = UserRepository()
admin = repo.get_user("admin")
if not admin:
    repo.create_user(UserCreate(username="admin", password="changeme"))
