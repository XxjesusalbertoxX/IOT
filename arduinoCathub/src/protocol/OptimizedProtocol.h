#ifndef OPTIMIZED_PROTOCOL_H
#define OPTIMIZED_PROTOCOL_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Estructura para comandos del arenero (legible)
struct LitterboxCommand {
  String action;             // "move", "block", "unblock", "disable", "enable"
  float degrees;             // Grados a mover
  String state;              // "empty", "ready", "blocked"
  bool blocked;              // Motor bloqueado
  bool enabled;              // Motor habilitado
  int cleaningInterval;      // Intervalo en minutos
};

// Estructura para respuestas
struct StatusResponse {
  String device;             // "litterbox", "feeder", "water"
  String status;             // "ok", "error", "busy", "blocked"
  String message;            // Mensaje adicional
  float value;               // Valor opcional
};

class OptimizedProtocol {
private:
  static const size_t JSON_BUFFER_SIZE = 256;  // Buffer más pequeño pero suficiente
  
public:
  // Parsear comando JSON optimizado
  bool parseCommand(const String& jsonString, LitterboxCommand& command);
  
  // Enviar respuesta legible
  void sendResponse(const StatusResponse& response);
  void sendError(const String& device, const String& error);
  void sendOK(const String& device, const String& message = "");
  
  // Validar formato del comando
  bool isValidCommand(const String& jsonString);
};

#endif