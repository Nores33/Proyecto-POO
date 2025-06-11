from sqlalchemy import Column, Integer, String, Text, DateTime, ForeignKey, func
from sqlalchemy.orm import relationship
from database import Base

class Historial(Base):
    __tablename__ = "historial"

    id = Column(Integer, primary_key=True, index=True)
    usuario_id = Column(Integer, ForeignKey("usuarios.id", ondelete="CASCADE"), nullable=False)
    nombre_archivo = Column(String(255), nullable=False)
    transcripcion = Column(Text, nullable=False)
    fecha_modificacion = Column(DateTime, server_default=func.now(), onupdate=func.now())

    usuario = relationship("Usuario", backref="historiales")