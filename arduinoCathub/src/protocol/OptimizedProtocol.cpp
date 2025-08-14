#include "OptimizedProtocol.h"

bool OptimizedProtocol::parseCommand(const String& jsonString, LitterboxCommand& command) {
  StaticJsonDocument<JSON_BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) return false;
  
  command.action = doc["action"] | "";
  command.degrees = doc["degrees"] | 0.0;
  command.state = doc["state"] | "";
  command.blocked = doc["blocked"] | false;
  command.enabled = doc["enabled"] | true;
  command.cleaningInterval = doc["cleaningInterval"] | 30;
  
  return true;
}

void OptimizedProtocol::sendResponse(const StatusResponse& response) {
  StaticJsonDocument<JSON_BUFFER_SIZE> doc;
  
  doc["device"] = response.device;
  doc["status"] = response.status;
  doc["message"] = response.message;
  
  if (response.value != 0) {
    doc["value"] = response.value;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void OptimizedProtocol::sendError(const String& device, const String& error) {
  StatusResponse response;
  response.device = device;
  response.status = "error";
  response.message = error;
  response.value = 0;
  
  sendResponse(response);
}

void OptimizedProtocol::sendOK(const String& device, const String& message) {
  StatusResponse response;
  response.device = device;
  response.status = "ok";
  response.message = message;
  response.value = 0;
  
  sendResponse(response);
}

bool OptimizedProtocol::isValidCommand(const String& jsonString) {
  return jsonString.startsWith("{") && jsonString.endsWith("}");
}