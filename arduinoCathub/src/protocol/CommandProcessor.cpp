#include "CommandProcessor.h"
#include "../config/MotorConfigs.h"

CommandProcessor::CommandProcessor(LitterboxStepperMotor* lbMotor) 
  : litterboxMotor(lbMotor) {
}

void CommandProcessor::initialize() {
  Serial.println("[CommandProcessor] Inicializado - Esperando comandos JSON");
}

void CommandProcessor::processCommand(const String& rawCommand) {
  // Validar formato básico
  if (!protocol.isValidCommand(rawCommand)) {
    protocol.sendError("system", "Invalid command format");
    return;
  }
  
  // Determinar tipo de dispositivo por contenido
  if (rawCommand.indexOf("litterbox") >= 0 || rawCommand.indexOf("degrees") >= 0) {
    processLitterboxCommand(rawCommand);
  }
  else if (rawCommand.indexOf("feeder") >= 0) {
    processFeederCommand(rawCommand);
  }
  else if (rawCommand.indexOf("emergency") >= 0) {
    processEmergencyCommand(rawCommand);
  }
  else {
    protocol.sendError("system", "Unknown device command");
  }
}

void CommandProcessor::processLitterboxCommand(const String& jsonCommand) {
  LitterboxCommand command;
  
  if (!protocol.parseCommand(jsonCommand, command)) {
    protocol.sendError("litterbox", "Failed to parse command");
    return;
  }
  
  // Procesar según la acción
  bool success = false;
  String resultMessage = "";
  
  if (command.action == "move") {
    success = litterboxMotor->moveToPosition(command.degrees);
    resultMessage = "Moving " + String(command.degrees) + " degrees";
  }
  else if (command.action == "block") {
    success = litterboxMotor->setBlocked(true);
    resultMessage = "Motor blocked";
  }
  else if (command.action == "unblock") {
    success = litterboxMotor->setBlocked(false);
    resultMessage = "Motor unblocked";
  }
  else if (command.action == "disable") {
    success = litterboxMotor->setEnabled(false);
    resultMessage = "Motor disabled (no torque)";
  }
  else if (command.action == "enable") {
    success = litterboxMotor->setEnabled(true);
    resultMessage = "Motor enabled";
  }
  else if (command.action == "setState") {
    success = litterboxMotor->setState(command.state);
    resultMessage = "State set to " + command.state;
  }
  else if (command.action == "setInterval") {
    success = litterboxMotor->setCleaningInterval(command.cleaningInterval);
    resultMessage = "Cleaning interval set to " + String(command.cleaningInterval) + " minutes";
  }
  else {
    protocol.sendError("litterbox", "Unknown action: " + command.action);
    return;
  }
  
  // Enviar respuesta
  if (success) {
    protocol.sendOK("litterbox", resultMessage);
  } else {
    protocol.sendError("litterbox", "Failed to execute: " + command.action);
  }
}

void CommandProcessor::processEmergencyCommand(const String& jsonCommand) {
  litterboxMotor->emergencyStop();
  protocol.sendOK("system", "Emergency stop executed");
}

void CommandProcessor::update() {
  // Actualizar todos los motores
  litterboxMotor->update();
}