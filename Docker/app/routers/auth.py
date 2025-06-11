import os
import hashlib
from datetime import timedelta, datetime
from jose import jwt, JWTError
from fastapi import APIRouter, Depends, HTTPException, status, Form, Header
from fastapi.security import HTTPBearer, HTTPAuthorizationCredentials
from sqlalchemy.orm import Session

from models.usuarios import Usuario
from models.historial import Historial
from database import get_db

# 🔐 Configuración desde variables de entorno
ACCESS_TOKEN_EXPIRE_MINUTES = os.getenv("ACCESS_TOKEN_EXPIRE_MINUTES")
SECRET_KEY = os.getenv("SECRET_KEY")
ALGORITHM = os.getenv("JWT_ALGORITHM")

# 🚨 Verificación obligatoria de variables de entorno
if not ACCESS_TOKEN_EXPIRE_MINUTES or not SECRET_KEY or not ALGORITHM:
    raise RuntimeError("Faltan variables requeridas en el entorno (.env): ACCESS_TOKEN_EXPIRE_MINUTES, SECRET_KEY, JWT_ALGORITHM")

ACCESS_TOKEN_EXPIRE_MINUTES = int(ACCESS_TOKEN_EXPIRE_MINUTES)

router = APIRouter()
security = HTTPBearer()

# Función para hashear usando MD5 (solo se recomienda si las claves ya están así guardadas)
def hash_md5(password: str) -> str:
    return hashlib.md5(password.encode()).hexdigest()

@router.post("/login", response_model=dict)
def login(
    username: str = Form(...),
    password: str = Form(...),
    db: Session = Depends(get_db)
):
    user = db.query(Usuario).filter(Usuario.usuario == username).first()

    if not user or user.clave != hash_md5(password):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Credenciales inválidas",
        )

    expire = datetime.utcnow() + timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    token_data = {"sub": user.usuario, "exp": expire}
    token = jwt.encode(token_data, SECRET_KEY, algorithm=ALGORITHM)

    return {
        "access_token": token,
        "token_type": "bearer",
        "usuario": user.usuario,
        "nombre": user.nombre,
        "apellido": user.apellido,
    }



@router.post("/registro", response_model=dict)
def registrar_usuario(
    nombre: str = Form(...),
    apellido: str = Form(...),
    usuario: str = Form(...),
    clave: str = Form(...),
    mail: str = Form(...),
    db: Session = Depends(get_db)
):
    # Verificar si ya existe un usuario con el mismo nombre de usuario o correo
    if db.query(Usuario).filter(Usuario.usuario == usuario).first():
        raise HTTPException(
            status_code=400,
            detail="El nombre de usuario ya está en uso."
        )
    if db.query(Usuario).filter(Usuario.mail == mail).first():
        raise HTTPException(
            status_code=400,
            detail="El correo electrónico ya está en uso."
        )

    # Crear nuevo usuario
    nuevo_usuario = Usuario(
        nombre=nombre,
        apellido=apellido,
        usuario=usuario,
        clave=hash_md5(clave),  # Se mantiene MD5 porque tu base ya lo usa así
        mail=mail
    )

    db.add(nuevo_usuario)
    db.commit()
    db.refresh(nuevo_usuario)

    return {
        "mensaje": "Usuario creado exitosamente.",
        "usuario_id": nuevo_usuario.id,
        "usuario": nuevo_usuario.usuario
    }


@router.post("/cambiar-clave", response_model=dict)
def cambiar_clave(
    usuario: str = Form(...),
    clave_actual: str = Form(...),
    nueva_clave: str = Form(...),
    db: Session = Depends(get_db)
):
    # Buscar usuario
    user = db.query(Usuario).filter(Usuario.usuario == usuario).first()
    if not user:
        raise HTTPException(status_code=404, detail="Usuario no encontrado.")

    # Verificar clave actual
    if user.clave != hash_md5(clave_actual):
        raise HTTPException(status_code=401, detail="La contraseña actual es incorrecta.")

    # Actualizar clave
    user.clave = hash_md5(nueva_clave)
    db.commit()

    return {
        "mensaje": "Contraseña actualizada correctamente."
    }


# CRUD Historial

@router.get("/historial", response_model=list[dict])
def leer_historial(
    usuario: str,
    db: Session = Depends(get_db)
):
    user = db.query(Usuario).filter(Usuario.usuario == usuario).first()
    if not user:
        raise HTTPException(status_code=404, detail="Usuario no encontrado")
    historial = db.query(Historial).filter(Historial.usuario_id == user.id).all()
    return [
        {
            "id": h.id,
            "nombre_archivo": h.nombre_archivo,
            "transcripcion": h.transcripcion,
            "fecha_modificacion": h.fecha_modificacion
        } for h in historial
    ]

@router.post("/historial", response_model=dict)
def agregar_historial(
    usuario: str = Form(...),
    nombre_archivo: str = Form(...),
    transcripcion: str = Form(...),
    db: Session = Depends(get_db)
):
    user = db.query(Usuario).filter(Usuario.usuario == usuario).first()
    if not user:
        raise HTTPException(status_code=404, detail="Usuario no encontrado")
    nuevo = Historial(
        usuario_id=user.id,
        nombre_archivo=nombre_archivo,
        transcripcion=transcripcion
    )
    db.add(nuevo)
    db.commit()
    db.refresh(nuevo)
    return {
        "id": nuevo.id,
        "nombre_archivo": nuevo.nombre_archivo,
        "transcripcion": nuevo.transcripcion,
        "fecha_modificacion": nuevo.fecha_modificacion
    }

@router.put("/historial/{historial_id}", response_model=dict)
def modificar_historial(
    historial_id: int,
    usuario: str = Form(...),
    nombre_archivo: str = Form(None),
    transcripcion: str = Form(None),
    db: Session = Depends(get_db)
):
    user = db.query(Usuario).filter(Usuario.usuario == usuario).first()
    if not user:
        raise HTTPException(status_code=404, detail="Usuario no encontrado")
    historial = db.query(Historial).filter(Historial.id == historial_id, Historial.usuario_id == user.id).first()
    if not historial:
        raise HTTPException(status_code=404, detail="Historial no encontrado")
    if nombre_archivo is not None:
        historial.nombre_archivo = nombre_archivo
    if transcripcion is not None:
        historial.transcripcion = transcripcion
    db.commit()
    db.refresh(historial)
    return {
        "id": historial.id,
        "nombre_archivo": historial.nombre_archivo,
        "transcripcion": historial.transcripcion,
        "fecha_modificacion": historial.fecha_modificacion
    }

@router.delete("/historial/{historial_id}", response_model=dict)
def borrar_historial(
    historial_id: int,
    usuario: str,
    db: Session = Depends(get_db)
):
    user = db.query(Usuario).filter(Usuario.usuario == usuario).first()
    if not user:
        raise HTTPException(status_code=404, detail="Usuario no encontrado")
    historial = db.query(Historial).filter(Historial.id == historial_id, Historial.usuario_id == user.id).first()
    if not historial:
        raise HTTPException(status_code=404, detail="Historial no encontrado")
    db.delete(historial)
    db.commit()
    return {"mensaje": "Historial eliminado correctamente."}

# Endpoint seguro para obtener la API key de OpenAI
@router.get("/openai-key", response_model=dict)
def obtener_openai_key(
    usuario: str,
    db: Session = Depends(get_db)
):
    user = db.query(Usuario).filter(Usuario.usuario == usuario).first()
    if not user:
        raise HTTPException(status_code=404, detail="Usuario no encontrado")
    openai_key = os.getenv("OPENAI_API_KEY")
    if not openai_key:
        raise HTTPException(status_code=500, detail="API key de OpenAI no configurada")
    return {"openai_key": openai_key}
