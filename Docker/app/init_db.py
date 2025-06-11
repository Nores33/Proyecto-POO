import os
import hashlib
from database import Base, engine, SessionLocal
from models.usuarios import Usuario
from models.historial import Historial

def hash_md5(password: str) -> str:
    return hashlib.md5(password.encode()).hexdigest()

def init():
    Base.metadata.create_all(bind=engine)
