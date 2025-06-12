# Transcriptor de conversaciones

Aplicación de transcripción de conversaciones a texto con historial de transcripciones, autenticación de usuarios y exportación de resultados. El proyecto está dividido en dos partes principales:

- **Frontend (Qt/C++):** Interfaz gráfica de usuario para grabar, cargar y transcribir audios, gestionar usuarios y visualizar/exportar el historial.
- **Backend (FastAPI + MySQL):** API REST para autenticación, gestión de usuarios, historial y conexión con OpenAI Whisper para transcripción.

## Características

- Registro e inicio de sesión de usuarios.
- Grabación y carga de archivos de audio (.wav, .mp3).
- Transcripción automática usando la API de OpenAI Whisper.
- Historial de transcripciones por usuario.
- Exportación de transcripciones a TXT, PDF y Word.
- Gestión de contraseñas y datos de usuario.
- Interfaz moderna y fácil de usar (Qt Widgets).
- Backend dockerizado con base de datos MySQL y panel phpMyAdmin.

## Estructura del proyecto

```
Proyecto-POO/
├── Aplicacion/           # Código fuente de la app Qt/C++
├── Docker/
│   ├── app/              # Backend FastAPI + modelos + routers
│   ├── docker-compose.yml
│   ├── Dockerfile
│   └── wait-for-it.sh
├── README.md
└── .gitignore
```

## Requisitos

- **Frontend:** Qt 6.x (o superior), compilador C++17.
- **Backend:** Docker y Docker Compose.
- **API de OpenAI:** Necesitas una API Key válida para Whisper.

## Instalación y ejecución

### Backend (API + DB)

1. Crea el archivo `.env` en `Docker/` y configura tus variables (MySQL, claves, etc) de la siguiente forma:

    ```
    MYSQL_ROOT_PASSWORD=
    MYSQL_DATABASE=poo_db
    MYSQL_USER=
    MYSQL_PASSWORD=
    DEFAULT_USER_PASSWORD=
    SECRET_KEY=
    ACCESS_TOKEN_EXPIRE_MINUTES=60
    JWT_ALGORITHM=HS256
    MYSQL_HOST=poo_mysql
    OPENAI_API_KEY=
    ```

2. Desde la carpeta `Docker/`, ejecuta:

   ```sh
   docker-compose up --build
   ```

   Esto levantará:
   - MySQL (puerto 3306)
   - phpMyAdmin (puerto 8080)
   - FastAPI backend (puerto 8000)

3. Accede a la API en [http://localhost:8000/docs](http://localhost:8000/docs) para ver la documentación interactiva.

### Frontend (Qt/C++)

1. Abre el proyecto en Qt Creator o tu IDE favorito.
2. Compila el proyecto (`LoginAWS.pro`).
3. Ejecuta el binario generado.
4. La primera vez, la app te pedirá la URL del backend (ejemplo: `http://localhost:8000`).

## Uso

- **Registro:** Haz clic en "Regístrate aquí" en la pantalla de login.
- **Login:** Ingresa tus credenciales.
- **Transcribir:** Graba o carga un archivo de audio y espera la transcripción.
- **Historial:** Consulta, exporta o elimina transcripciones anteriores.
- **Exportar:** Elige el formato deseado (TXT, PDF, Word).

## Variables de entorno importantes (backend)

- `MYSQL_USER`, `MYSQL_PASSWORD`, `MYSQL_DATABASE`, `MYSQL_HOST`
- `SECRET_KEY`, `ACCESS_TOKEN_EXPIRE_MINUTES`, `JWT_ALGORITHM`
- `OPENAI_API_KEY`

## Créditos

- Qt/C++ para la interfaz gráfica.
- FastAPI, SQLAlchemy y MySQL para el backend.
- OpenAI Whisper para transcripción automática.

---

> Proyecto realizado para la materia Programación Orientada a Objetos.
