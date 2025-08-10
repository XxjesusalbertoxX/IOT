from sensors.sensor import Sensor
import cv2
import numpy as np
from datetime import datetime
import os

class SensorCamara(Sensor):
    def __init__(self, id, valor_min=0, valor_max=1):
        super().__init__(id, valor_min, valor_max, unidad="")
        self.camara_activa = False
        self.ultima_foto_path = None
        self.detectar_gato = True
        self.contador_detecciones = 0
        
        # Configuración de OpenCV para detección
        self.cascade_gato = None
        try:
            # Intentar cargar clasificador para detección de gatos/animales
            cascade_path = cv2.data.haarcascades + 'haarcascade_frontalface_alt.xml'
            if os.path.exists(cascade_path):
                self.cascade_gato = cv2.CascadeClassifier(cascade_path)
        except Exception as e:
            print(f"No se pudo cargar clasificador de detección: {e}")
    
    def leer_valor(self):
        """Leer/capturar imagen y detectar presencia"""
        try:
            if self.detectar_gato:
                return self._detectar_presencia_gato()
            else:
                return self._capturar_imagen()
                
        except Exception as e:
            print(f"Error en sensor de cámara {self.id}: {e}")
            return 0
    
    def _capturar_imagen(self):
        """Capturar imagen desde la cámara"""
        try:
            cap = cv2.VideoCapture(0)  # Cámara por defecto
            
            if not cap.isOpened():
                print(f"No se puede abrir cámara para sensor {self.id}")
                return 0
            
            ret, frame = cap.read()
            cap.release()
            
            if ret:
                # Guardar imagen con timestamp
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"gato_{self.id}_{timestamp}.jpg"
                photos_dir = os.path.join(os.getcwd(), "fotos_gatos")
                os.makedirs(photos_dir, exist_ok=True)
                
                filepath = os.path.join(photos_dir, filename)
                cv2.imwrite(filepath, frame)
                self.ultima_foto_path = filepath
                
                print(f"📸 Foto capturada: {filename}")
                self.guardar_lectura(1)
                return 1
            
            return 0
            
        except Exception as e:
            print(f"Error capturando imagen: {e}")
            return 0
    
    def _detectar_presencia_gato(self):
        """Detectar presencia de gato usando visión computacional"""
        try:
            cap = cv2.VideoCapture(0)
            
            if not cap.isOpened():
                return 0
            
            ret, frame = cap.read()
            cap.release()
            
            if not ret:
                return 0
            
            # Convertir a escala de grises para detección
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            
            # Método simple: detección de movimiento/cambio
            deteccion = self._detectar_movimiento(frame, gray)
            
            if deteccion:
                self.contador_detecciones += 1
                # Guardar imagen cuando hay detección
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"deteccion_gato_{self.id}_{timestamp}.jpg"
                photos_dir = os.path.join(os.getcwd(), "detecciones_gatos")
                os.makedirs(photos_dir, exist_ok=True)
                
                filepath = os.path.join(photos_dir, filename)
                cv2.imwrite(filepath, frame)
                self.ultima_foto_path = filepath
                
                print(f"🐱 Gato detectado por cámara {self.id}")
                self.guardar_lectura(1)
                return 1
            
            self.guardar_lectura(0)
            return 0
            
        except Exception as e:
            print(f"Error en detección: {e}")
            return 0
    
    def _detectar_movimiento(self, frame, gray):
        """Método simple de detección de movimiento"""
        # Guardar frame anterior para comparación
        if hasattr(self, '_frame_anterior'):
            # Calcular diferencia entre frames
            diff = cv2.absdiff(self._frame_anterior, gray)
            _, thresh = cv2.threshold(diff, 30, 255, cv2.THRESH_BINARY)
            
            # Contar píxeles que cambiaron
            pixels_cambiados = cv2.countNonZero(thresh)
            total_pixels = gray.shape[0] * gray.shape[1]
            porcentaje_cambio = (pixels_cambiados / total_pixels) * 100
            
            # Si hay más de 5% de cambio, considerar movimiento
            if porcentaje_cambio > 5:
                self._frame_anterior = gray.copy()
                return True
        
        self._frame_anterior = gray.copy()
        return False
    
    def capturar_foto_manual(self):
        """Capturar foto manualmente"""
        return self._capturar_imagen()
    
    def obtener_info_completa(self):
        """Obtener información completa del sensor"""
        return {
            'id': self.id,
            'camara_activa': self.camara_activa,
            'ultima_foto': self.ultima_foto_path,
            'contador_detecciones': self.contador_detecciones,
            'detectar_gato': self.detectar_gato,
            'estado': self.estado,
            'ultima_lectura': self.ultima_lectura.isoformat() if self.ultima_lectura else None
        }
    
    def actualizar_estado(self, valor):
        """Actualizar estado específico para cámara"""
        if valor is None:
            self.estado = "error_lectura"
        elif valor == 1:
            self.estado = "activa_deteccion"
        else:
            self.estado = "sin_deteccion"
    
    def __str__(self):
        return f"Cámara {self.id}: {'Activa' if self.valor_actual == 1 else 'Sin detección'} - Detecciones: {self.contador_detecciones}"
