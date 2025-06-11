from fastapi import FastAPI
from routers import auth
import init_db
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI()

# 🚨 Agrega esto antes de incluir los routers
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Cambiá esto a ["http://localhost:3000"] si usás frontend en React por ejemplo
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Inicializa DB y carga usuarios si no existen
init_db.init()

app.include_router(auth.router, prefix="/auth", tags=["Autenticación"])
