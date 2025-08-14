#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "../sensors/SensorManager.h"
#include "../sensors/litterbox/LitterboxStepperMotor.h"
#include "OptimizedProtocol.h"

class CommandProcessor {
private:
  SensorManager* sensorManager;
  LitterboxStepperMotor* litterboxMotor;
  OptimizedProtocol protocol;
  
  // Procesar comandos específicos por dispositivo
  void processLitterboxCommand(const String& jsonCommand);
  void processFeederCommand(const String& jsonCommand);
  void processWaterCommand(const String& jsonCommand);
  void processEmergencyCommand(const String& jsonCommand);
  
  // Procesar solicitudes de datos específicos
  void processSensorRequest(const String& jsonCommand);
  
  // Funciones para enviar datos específicos
  void enviarDatosLitterboxPresencia();
  void enviarDatosLitterboxAmbiente();
  void enviarDatosFeederPeso();
  void enviarDatosFeederNivel();
  void enviarDatosWaterNivel();
  void enviarDatosWaterIR();
  void enviarTodosLosDatos();
  
public:
  CommandProcessor(SensorManager* sm, LitterboxStepperMotor* lbMotor);
  
  void initialize();
  void processCommand(const String& jsonCommand);
  void update(); // Llamar en loop()
};

#endif