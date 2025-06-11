# Usa una imagen oficial con Python 3.11
FROM python:3.11-slim

# Establecer directorio de trabajo dentro del contenedor
WORKDIR /app

# Copiar solo los archivos de requerimientos primero (para cachear instalación de paquetes)
COPY app/requirements.txt .

# Instalar dependencias
RUN pip install --no-cache-dir -r requirements.txt

# Copiar el resto del código de la app
COPY app/ .

# Agregar script para esperar a que cargue la db
COPY wait-for-it.sh /wait-for-it.sh
RUN chmod +x /wait-for-it.sh

# Exponer el puerto usado por Uvicorn
EXPOSE 8000

# Comando por defecto para ejecutar FastAPI
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000"]
