#include "CommandProcessor.h"
#include "../config/MotorConfigs.h"
#include <ArduinoJson.h>

CommandProcessor::CommandProcessor(SensorManager* sm, LitterboxStepperMotor* lbMotor) 
  : sensorManager(sm), litterboxMotor(lbMotor) {
}

void CommandProcessor::initialize() {
  Serial.println("[CommandProcessor] Inicializado - Esperando comandos de la Ras");
}

void CommandProcessor::processCommand(const String& rawCommand) {
  // Validar formato básico JSON
  if (!rawCommand.startsWith("{") || !rawCommand.endsWith("}")) {
    protocol.sendError("system", "Invalid JSON format");
    return;
  }
  
  // Parsear JSON - ✅ CORREGIDO: usar JsonDocument
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, rawCommand);
  
  if (error) {
    protocol.sendError("system", "JSON parse error");
    return;
  }
  
  String type = doc["type"] | "";
  
  // Determinar tipo de comando
  if (type == "REQUEST_SENSOR_DATA") {
    processSensorRequest(rawCommand);
  }
  else if (type == "LITTERBOX_COMMAND") {
    processLitterboxCommand(rawCommand);
  }
  else if (type == "FEEDER_COMMAND") {
    processFeederCommand(rawCommand);
  }
  else if (type == "WATER_COMMAND") {
    processWaterCommand(rawCommand);
  }
  else if (type == "EMERGENCY_STOP") {
    processEmergencyCommand(rawCommand);
  }
  else {
    protocol.sendError("system", "Unknown command type: " + type);
  }
}

// === PROCESAMIENTO DE SOLICITUDES DE SENSORES ===
void CommandProcessor::processSensorRequest(const String& jsonCommand) {
  JsonDocument doc;  // ✅ CORREGIDO
  deserializeJson(doc, jsonCommand);
  
  String sensor = doc["sensor"] | "all";
  
  if (sensor == "litterbox_presence") {
    enviarDatosLitterboxPresencia();
  }
  else if (sensor == "litterbox_environment") {
    enviarDatosLitterboxAmbiente();
  }
  else if (sensor == "feeder_weight") {
    enviarDatosFeederPeso();
  }
  else if (sensor == "feeder_level") {
    enviarDatosFeederNivel();
  }
  else if (sensor == "water_level") {
    enviarDatosWaterNivel();
  }
  else if (sensor == "water_ir") {
    enviarDatosWaterIR();
  }
  else if (sensor == "all") {
    enviarTodosLosDatos();
  }
  else {
    protocol.sendError("sensors", "Unknown sensor: " + sensor);
  }
}

// === PROCESAMIENTO DE COMANDOS DE ACTUADORES ===
void CommandProcessor::processLitterboxCommand(const String& jsonCommand) {
  JsonDocument doc;  // ✅ CORREGIDO
  deserializeJson(doc, jsonCommand);
  
  String action = doc["action"] | "";
  
  bool success = false;
  String message = "";
  
  if (action == "fill_litter") {
    success = litterboxMotor->fillWithLitter();
    message = success ? "Arena agregada - Estado READY" : "Error llenando arena";
  }
  else if (action == "normal_cleaning") {
    success = litterboxMotor->executeNormalCleaning();
    message = success ? "Limpieza normal completada" : "Error en limpieza normal";
  }
  else if (action == "complete_cleaning") {
    success = litterboxMotor->executeCompleteCleaning();
    message = success ? "Limpieza completa - Estado EMPTY" : "Error en limpieza completa";
  }
  else if (action == "block") {
    success = litterboxMotor->blockMotor();
    message = "Motor bloqueado por seguridad";
  }
  else if (action == "unblock") {
    success = litterboxMotor->unblockMotor();
    message = "Motor desbloqueado";
  }
  else if (action == "get_status") {
    protocol.sendOK("litterbox", litterboxMotor->getStatus());
    return;
  }
  else if (action == "emergency_stop") {
    litterboxMotor->emergencyStop();
    success = true;
    message = "PARADA DE EMERGENCIA ejecutada";
  }
  else {
    protocol.sendError("litterbox", "Acción desconocida: " + action);
    return;
  }
  
  if (success) {
    protocol.sendOK("litterbox", message);
  } else {
    protocol.sendError("litterbox", "Error ejecutando: " + action);
  }
}

void CommandProcessor::processFeederCommand(const String& jsonCommand) {
  JsonDocument doc;  // ✅ CORREGIDO
  deserializeJson(doc, jsonCommand);
  
  String action = doc["action"] | "";
  
  // Aquí implementarías comandos del comedero
  // Por ahora solo responder que se recibió
  protocol.sendOK("feeder", "Comando recibido: " + action);
}

void CommandProcessor::processWaterCommand(const String& jsonCommand) {
  JsonDocument doc;  // ✅ CORREGIDO
  deserializeJson(doc, jsonCommand);
  
  String action = doc["action"] | "";
  
  // Aquí implementarías comandos del bebedero
  // Por ahora solo responder que se recibió
  protocol.sendOK("water", "Comando recibido: " + action);
}

void CommandProcessor::processEmergencyCommand(const String& jsonCommand) {
  // Parar todos los motores
  if (litterboxMotor) {
    litterboxMotor->emergencyStop();
  }
  
  // Parar bomba de agua si está funcionando
  WaterDispenserPump* waterPump = sensorManager->getWaterDispenserPump();
  if (waterPump) {
    waterPump->emergencyStop();
  }
  
  protocol.sendOK("system", "Parada de emergencia ejecutada en todos los dispositivos");
}

// === FUNCIONES DE ENVÍO DE DATOS DE SENSORES ===
void CommandProcessor::enviarDatosLitterboxPresencia() {
  JsonDocument doc;  // ✅ CORREGIDO
  
  doc["sensor"] = "litterbox_presence";
  doc["timestamp_ms"] = millis();
  
  LitterboxUltrasonicSensor* sensor = sensorManager->getLitterboxUltrasonic();
  if (sensor && sensor->isReady()) {
    doc["distancia_cm"] = sensor->getDistance();
    doc["status"] = sensor->getStatus();
  } else {
    doc["distancia_cm"] = nullptr;
    doc["status"] = "ERROR";
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void CommandProcessor::enviarDatosLitterboxAmbiente() {
  JsonDocument doc;  // ✅ CORREGIDO
  
  doc["sensor"] = "litterbox_environment";
  doc["timestamp_ms"] = millis();
  
  // DHT Sensor
  LitterboxDHTSensor* dht = sensorManager->getLitterboxDHT();
  if (dht && dht->isReady()) {
    doc["temperatura_c"] = dht->getTemperature();
    doc["humedad_pct"] = dht->getHumidity();
  } else {
    doc["temperatura_c"] = nullptr;
    doc["humedad_pct"] = nullptr;
  }
  
  // MQ2 Sensor
  LitterboxMQ2Sensor* mq2 = sensorManager->getLitterboxMQ2();
  if (mq2 && mq2->isReady()) {
    doc["gas_ppm"] = mq2->getPPM();
    doc["gas_analog"] = mq2->getValue();
  } else {
    doc["gas_ppm"] = nullptr;
    doc["gas_analog"] = nullptr;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void CommandProcessor::enviarDatosFeederPeso() {
  JsonDocument doc;  // ✅ CORREGIDO
  doc["sensor"] = "feeder_weight";
  doc["timestamp_ms"] = millis();
  
  FeederWeightSensor* sensor = sensorManager->getFeederWeight();
  if (sensor && sensor->isReady()) {
    doc["peso_gramos"] = sensor->getCurrentWeight();
    doc["status"] = sensor->getStatus();
  } else {
    doc["peso_gramos"] = nullptr;
    doc["status"] = "ERROR";
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void CommandProcessor::enviarDatosFeederNivel() {
  JsonDocument doc;  // ✅ CORREGIDO
  
  doc["sensor"] = "feeder_level";
  doc["timestamp_ms"] = millis();
  
  // Ultrasónico 1
  FeederUltrasonicSensor1* us1 = sensorManager->getFeederUltrasonic1();
  if (us1 && us1->isReady()) {
    doc["nivel_comida_cm"] = us1->getDistance();
    doc["sensor1_status"] = us1->getStatus();
  } else {
    doc["nivel_comida_cm"] = nullptr;
    doc["sensor1_status"] = "ERROR";
  }
  
  // Ultrasónico 2  
  FeederUltrasonicSensor2* us2 = sensorManager->getFeederUltrasonic2();
  if (us2 && us2->isReady()) {
    doc["altura_contenedor_cm"] = us2->getDistance();
    doc["sensor2_status"] = us2->getStatus();
  } else {
    doc["altura_contenedor_cm"] = nullptr;
    doc["sensor2_status"] = "ERROR";
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void CommandProcessor::enviarDatosWaterNivel() {
  JsonDocument doc;  // ✅ CORREGIDO
  
  doc["sensor"] = "water_level";
  doc["timestamp_ms"] = millis();
  
  WaterDispenserSensor* sensor = sensorManager->getWaterDispenser();
  if (sensor && sensor->isReady()) {
    doc["nivel_analogico"] = sensor->getAnalogValue();
    doc["nivel_categoria"] = sensor->getWaterLevel();
    doc["agua_detectada"] = sensor->isWaterDetected();
  } else {
    doc["nivel_analogico"] = nullptr;
    doc["nivel_categoria"] = "ERROR";
    doc["agua_detectada"] = false;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void CommandProcessor::enviarDatosWaterIR() {
  JsonDocument doc;  // ✅ CORREGIDO
  
  doc["sensor"] = "water_ir";
  doc["timestamp_ms"] = millis();
  
  WaterDispenserIRSensor* sensor = sensorManager->getWaterDispenserIR();
  if (sensor && sensor->isReady()) {
    doc["gato_presente"] = sensor->isObjectDetected();
    doc["duracion_deteccion_ms"] = sensor->getDetectionDuration();
  } else {
    doc["gato_presente"] = false;
    doc["duracion_deteccion_ms"] = 0;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void CommandProcessor::enviarTodosLosDatos() {
  JsonDocument doc;  // ✅ CORREGIDO
  
  doc["sensor"] = "all_sensors";
  doc["timestamp_ms"] = millis();
  
  // LITTERBOX
  LitterboxDHTSensor* ldht = sensorManager->getLitterboxDHT();
  LitterboxMQ2Sensor* lmq2 = sensorManager->getLitterboxMQ2();
  LitterboxUltrasonicSensor* lus = sensorManager->getLitterboxUltrasonic();
  
  if (ldht && ldht->isReady()) {
    doc["litterbox"]["temperatura_c"] = ldht->getTemperature();
    doc["litterbox"]["humedad_pct"] = ldht->getHumidity();
  }
  if (lmq2 && lmq2->isReady()) {
    doc["litterbox"]["gas_ppm"] = lmq2->getPPM();
    doc["litterbox"]["gas_analog"] = lmq2->getValue();
  }
  if (lus && lus->isReady()) {
    doc["litterbox"]["distancia_cm"] = lus->getDistance();
  }
  
  // FEEDER
  FeederWeightSensor* fw = sensorManager->getFeederWeight();
  FeederUltrasonicSensor1* fu1 = sensorManager->getFeederUltrasonic1();
  FeederUltrasonicSensor2* fu2 = sensorManager->getFeederUltrasonic2();
  
  if (fw && fw->isReady()) {
    doc["feeder"]["peso_gramos"] = fw->getCurrentWeight();
  }
  if (fu1 && fu1->isReady()) {
    doc["feeder"]["nivel_comida_cm"] = fu1->getDistance();
  }
  if (fu2 && fu2->isReady()) {
    doc["feeder"]["altura_contenedor_cm"] = fu2->getDistance();
  }
  
  // WATER
  WaterDispenserSensor* wd = sensorManager->getWaterDispenser();
  WaterDispenserIRSensor* wir = sensorManager->getWaterDispenserIR();
  
  if (wd && wd->isReady()) {
    doc["water"]["nivel_analogico"] = wd->getAnalogValue();
    doc["water"]["nivel_categoria"] = wd->getWaterLevel();
    doc["water"]["agua_detectada"] = wd->isWaterDetected();
  }
  if (wir && wir->isReady()) {
    doc["water"]["gato_presente"] = wir->isObjectDetected();
    doc["water"]["duracion_deteccion_ms"] = wir->getDetectionDuration();
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  Serial.println(jsonString);
}

void CommandProcessor::update() {
  // La Raspberry Pi hace las validaciones
  // Arduino solo ejecuta comandos cuando se soliciten
  // No llamar litterboxMotor->update() porque ya no existe
}