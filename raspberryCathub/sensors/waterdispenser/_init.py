"""
Water dispenser sensors package
"""

from .water_level_sensor import WaterLevelSensor
from .ir_sensor import WaterIRSensor

__all__ = [
    'WaterLevelSensor',
    'WaterIRSensor'
]