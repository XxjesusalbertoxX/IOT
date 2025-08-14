#ifndef MOTOR_CONFIGS_H
#define MOTOR_CONFIGS_H

// Configuraciones del Motor del Arenero
struct LitterboxMotorConfig {
  static const int STEPS_PER_REVOLUTION = 200;
  static const int MICROSTEPS = 16;
  static const int TOTAL_STEPS_PER_REV = STEPS_PER_REVOLUTION * MICROSTEPS; // 3200
  static const float DEFAULT_SPEED = 500.0;          // pasos/seg
  static const float DEFAULT_ACCELERATION = 200.0;   // aceleración
  static const float MAX_SPEED = 800.0;              // velocidad máxima
  static const int EMERGENCY_TIMEOUT_MS = 5000;      // timeout emergencia
};

// Configuraciones del Motor del Dispensador
struct FeederMotorConfig {
  static const int STEPS_PER_REVOLUTION = 200;
  static const int MICROSTEPS = 8;
  static const int TOTAL_STEPS_PER_REV = STEPS_PER_REVOLUTION * MICROSTEPS; // 1600
  static const float DEFAULT_SPEED = 300.0;
  static const float DEFAULT_ACCELERATION = 150.0;
  static const int STEPS_PER_GRAM = 80;               // pasos por gramo
};

// Configuraciones del Sistema de Comunicación
struct CommConfig {
  static const int BAUD_RATE = 115200;
  static const int JSON_BUFFER_SIZE = 512;
  static const int COMMAND_TIMEOUT_MS = 100;
  static const int MAX_COMMAND_LENGTH = 256;
};

#endif