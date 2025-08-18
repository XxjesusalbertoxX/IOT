#include "CommandProcessor.h"

CommandProcessor::CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* motor) 
    : sensorManager(sensors), litterboxMotor(motor), initialized(false), 
      thresholdsConfiguredByRaspberry(false), lastConfigUpdate(0) {
    
    // 🔒 INICIALIZAR CON VALORES POR DEFECTO SEGUROS
    initializeDefaultThresholds();
    
    // ✅ INICIALIZAR ARRAYS DE IDENTIFICADORES
    for (int i = 0; i < MAX_SENSORS; i++) {
        sensorIdentifiers[i] = "";
        sensorConfigured[i] = false;
    }
    deviceIdentifier = "";
    deviceConfigured = false;
}

bool CommandProcessor::initialize() {
    initializeDefaultThresholds();
    initialized = true;
    
    Serial.println("{\"system\":\"COMMAND_PROCESSOR_READY\",\"thresholds\":\"USING_DEFAULTS\",\"config\":" + getCurrentThresholds() + "}");
    return true;
}

void CommandProcessor::processCommand(String command) {
    command.trim();
    if (command.length() == 0) return;

    // RESPONDER A PING
    if (command == "PING") {
        Serial.println("PONG");
        return;
    }

    // RESPONDER A PING (JSON)
    if (command.startsWith("{") && command.endsWith("}")) {
        StaticJsonDocument<64> doc;
        DeserializationError error = deserializeJson(doc, command);
        if (!error && doc.containsKey("type") && doc["type"] == "PING") {
            Serial.println("{\"type\":\"PONG\"}");
            return;
        }
    }

    // Formato esperado: "DEVICE:COMMAND:PARAMS"
    int firstColon = command.indexOf(':');
    int secondColon = command.indexOf(':', firstColon + 1);
    
    if (firstColon == -1) {
        Serial.println("{\"error\":\"INVALID_COMMAND_FORMAT\",\"received\":\"" + command + "\"}");
        return;
    }

    String device = command.substring(0, firstColon);
    String cmd = (secondColon == -1) ? command.substring(firstColon + 1) : command.substring(firstColon + 1, secondColon);
    String params = (secondColon == -1) ? "" : command.substring(secondColon + 1);

    // ✅ ENRUTAR COMANDOS INCLUYENDO DEVICE Y SENSOR
    if (device == "DEVICE") {
        processDeviceCommand(cmd, params);
    } else if (device == "SENSOR") {
        processSensorCommand(cmd, params);
    } else if (device == "LITTERBOX") {
        processLitterboxCommand(cmd, params);
    } else if (device == "CONFIG") {
        processConfigCommand(cmd, params);
    } else if (device == "STATUS") {
        processStatusCommand();
    } else {
        Serial.println("{\"error\":\"UNKNOWN_DEVICE\",\"device\":\"" + device + "\"}");
    }
}

// =========================================
// 🆔 COMANDOS DE IDENTIFICACIÓN DE DISPOSITIVO
// =========================================

void CommandProcessor::processDeviceCommand(String command, String params) {
    if (command == "SET_ID") {
        if (params.length() > 0) {
            deviceIdentifier = params;
            deviceConfigured = true;
            
            Serial.println("{\"device\":\"DEVICE\",\"command\":\"SET_ID\",\"success\":true,\"id\":\"" + deviceIdentifier + "\"}");
            
            // 📢 NOTIFICAR CONFIGURACIÓN COMPLETA
            if (hasSensorsConfigured()) {
                Serial.println("{\"system\":\"DEVICE_FULLY_CONFIGURED\",\"device_id\":\"" + deviceIdentifier + "\",\"configured_sensors\":" + String(getConfiguredSensorCount()) + "}");
            }
        } else {
            Serial.println("{\"device\":\"DEVICE\",\"command\":\"SET_ID\",\"success\":false,\"error\":\"EMPTY_ID\"}");
        }
    } else if (command == "GET_ID") {
        Serial.println("{\"device\":\"DEVICE\",\"command\":\"GET_ID\",\"id\":\"" + deviceIdentifier + "\",\"configured\":" + String(deviceConfigured) + "}");
    } else {
        Serial.println("{\"device\":\"DEVICE\",\"error\":\"UNKNOWN_COMMAND\",\"command\":\"" + command + "\"}");
    }
}

void CommandProcessor::processSensorCommand(String command, String params) {
    if (command == "SET_ID") {
        // Formato: "sensor_index:identifier"
        int colonIndex = params.indexOf(':');
        if (colonIndex != -1) {
            int sensorIndex = params.substring(0, colonIndex).toInt();
            String identifier = params.substring(colonIndex + 1);
            
            if (sensorIndex >= 0 && sensorIndex < MAX_SENSORS && identifier.length() > 0) {
                sensorIdentifiers[sensorIndex] = identifier;
                sensorConfigured[sensorIndex] = true;
                
                Serial.println("{\"device\":\"SENSOR\",\"command\":\"SET_ID\",\"success\":true,\"index\":" + String(sensorIndex) + ",\"id\":\"" + identifier + "\"}");
                
                // 📢 NOTIFICAR SI TODOS LOS SENSORES ESTÁN CONFIGURADOS
                if (deviceConfigured && hasSensorsConfigured()) {
                    Serial.println("{\"system\":\"ALL_SENSORS_CONFIGURED\",\"device_id\":\"" + deviceIdentifier + "\",\"total_sensors\":" + String(getConfiguredSensorCount()) + "}");
                }
            } else {
                Serial.println("{\"device\":\"SENSOR\",\"command\":\"SET_ID\",\"success\":false,\"error\":\"INVALID_INDEX_OR_ID\"}");
            }
        } else {
            Serial.println("{\"device\":\"SENSOR\",\"command\":\"SET_ID\",\"success\":false,\"error\":\"INVALID_FORMAT\"}");
        }
    } else if (command == "GET_IDS") {
        sendAllSensorIdentifiers();
    } else if (command == "GET_READINGS") {
        if (deviceConfigured && hasSensorsConfigured()) {
            sendAllSensorReadingsWithIdentifiers();
        } else {
            Serial.println("{\"device\":\"SENSOR\",\"command\":\"GET_READINGS\",\"success\":false,\"error\":\"NOT_CONFIGURED\"}");
        }
    } else {
        Serial.println("{\"device\":\"SENSOR\",\"error\":\"UNKNOWN_COMMAND\",\"command\":\"" + command + "\"}");
    }
}

// =========================================
// 📊 MÉTODOS DE LECTURA CON IDENTIFICADORES
// =========================================

void CommandProcessor::sendAllSensorReadingsWithIdentifiers() {
    if (!deviceConfigured) {
        Serial.println("{\"error\":\"DEVICE_NOT_CONFIGURED\"}");
        return;
    }

    StaticJsonDocument<1024> doc;
    doc["device_id"] = deviceIdentifier;
    doc["timestamp"] = millis();
    doc["readings"] = JsonObject();
    
    JsonObject readings = doc["readings"];
    
    // 📏 LEER SENSORES CONFIGURADOS
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (sensorConfigured[i]) {
            float value = sensorManager->readSensor(i);
            readings[sensorIdentifiers[i]] = value;
        }
    }
    
    String output;
    serializeJson(doc, output);
    Serial.println(output);
}

void CommandProcessor::sendAllSensorIdentifiers() {
    StaticJsonDocument<512> doc;
    doc["device_id"] = deviceIdentifier;
    doc["configured"] = deviceConfigured;
    doc["sensors"] = JsonObject();
    
    JsonObject sensors = doc["sensors"];
    
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (sensorConfigured[i]) {
            sensors[String(i)] = sensorIdentifiers[i];
        }
    }
    
    String output;
    serializeJson(doc, output);
    Serial.println(output);
}

// =========================================
// 🏠 COMANDOS DE LITTERBOX
// =========================================

void CommandProcessor::processLitterboxCommand(String command, String params) {
    if (command == "SET_STATE") {
        int state = params.toInt();
        
        // ✅ VALIDAR CONDICIONES DE SEGURIDAD ANTES DE CAMBIAR ESTADO
        if (!isLitterboxSafeToOperate()) {
            Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"SET_STATE\",\"success\":false,\"error\":\"SAFETY_CONDITIONS_NOT_MET\",\"details\":" + getSafetyStatus() + ",\"validation_errors\":" + getValidationErrors() + "}");
            
            // 🚨 AUTO-BLOQUEAR si no estaba ya bloqueado
            if (!litterboxMotor->isBlocked()) {
                litterboxMotor->setState(-1); // BLOCKED
                Serial.println("{\"db_alert\":{\"type\":\"AUTO_BLOCKED\",\"device\":\"LITTERBOX\",\"reason\":\"SAFETY_VALIDATION_FAILED\",\"errors\":" + getValidationErrors() + ",\"timestamp\":" + String(millis()) + "}}");
            }
            return;
        }
        
        bool success = litterboxMotor->setState(state);
        Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"SET_STATE\",\"success\":" + String(success) + ",\"state\":" + String(state) + "}");
        
    } else if (command == "NORMAL_CLEAN") {
        // ✅ VALIDACIÓN CRÍTICA: No limpiar si hay gato o está sucio
        if (!isLitterboxSafeToClean()) {
            String errors = getValidationErrors();
            Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"NORMAL_CLEAN\",\"success\":false,\"error\":\"NOT_SAFE_TO_CLEAN\",\"details\":" + getSafetyStatus() + ",\"validation_errors\":" + errors + "}");
            
            // 🚨 REPORTAR ERROR ESPECÍFICO PARA LA DB
            Serial.println("{\"db_alert\":{\"type\":\"CLEANING_BLOCKED\",\"device\":\"LITTERBOX\",\"action\":\"NORMAL_CLEAN\",\"errors\":" + errors + ",\"timestamp\":" + String(millis()) + "}}");
            return;
        }
        
        bool success = litterboxMotor->performNormalCleaning();
        Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"NORMAL_CLEAN\",\"success\":" + String(success) + "}");
        
    } else if (command == "DEEP_CLEAN") {
        if (!isLitterboxSafeToClean()) {
            String errors = getValidationErrors();
            Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"DEEP_CLEAN\",\"success\":false,\"error\":\"NOT_SAFE_TO_CLEAN\",\"details\":" + getSafetyStatus() + ",\"validation_errors\":" + errors + "}");
            Serial.println("{\"db_alert\":{\"type\":\"CLEANING_BLOCKED\",\"device\":\"LITTERBOX\",\"action\":\"DEEP_CLEAN\",\"errors\":" + errors + ",\"timestamp\":" + String(millis()) + "}}");
            return;
        }
        
        bool success = litterboxMotor->performDeepCleaning();
        Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"DEEP_CLEAN\",\"success\":" + String(success) + "}");
        
    } else if (command == "GET_STATUS") {
        sendLitterboxStatus();
    } else {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"UNKNOWN_COMMAND\",\"command\":\"" + command + "\"}");
    }
}

// =========================================
// ⚙️ COMANDOS DE CONFIGURACIÓN
// =========================================

void CommandProcessor::processConfigCommand(String command, String params) {
    if (command == "SET_THRESHOLDS") {
        processThresholdCommand("SET_THRESHOLDS:" + params);
    } else if (command == "GET_THRESHOLDS") {
        sendCurrentConfig();
    } else if (command == "RESET") {
        resetToDefaults();
    } else {
        Serial.println("{\"device\":\"CONFIG\",\"error\":\"UNKNOWN_COMMAND\",\"command\":\"" + command + "\"}");
    }
}

void CommandProcessor::processStatusCommand() {
    StaticJsonDocument<1024> doc;
    doc["device_id"] = deviceIdentifier;
    doc["device_configured"] = deviceConfigured;
    doc["sensors_configured"] = getConfiguredSensorCount();
    doc["system_status"] = "OPERATIONAL";
    doc["uptime"] = millis();
    doc["litterbox_status"] = litterboxMotor->getCurrentState();
    doc["safety_status"] = getSafetyStatus();
    
    String output;
    serializeJson(doc, output);
    Serial.println(output);
}

// =========================================
// 🔧 MÉTODOS DE UTILIDAD
// =========================================

bool CommandProcessor::hasSensorsConfigured() {
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (sensorConfigured[i]) {
            return true;
        }
    }
    return false;
}

int CommandProcessor::getConfiguredSensorCount() {
    int count = 0;
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (sensorConfigured[i]) {
            count++;
        }
    }
    return count;
}

// =========================================
// 🛠️ MÉTODOS EXISTENTES (COMPATIBILIDAD)
// =========================================

void CommandProcessor::processThresholdCommand(String command) {
    // Extraer parámetros del comando SET_THRESHOLDS:param1,param2,...
    String params = command.substring(command.indexOf(':') + 1);
    
    // Parsear parámetros separados por comas
    int values[6] = {0}; // Para 6 posibles umbrales
    int valueIndex = 0;
    int startPos = 0;
    
    for (int i = 0; i <= params.length() && valueIndex < 6; i++) {
        if (i == params.length() || params.charAt(i) == ',') {
            if (i > startPos) {
                values[valueIndex] = params.substring(startPos, i).toInt();
                valueIndex++;
            }
            startPos = i + 1;
        }
    }
    
    // Aplicar umbrales si se proporcionaron valores válidos
    if (valueIndex >= 2) {
        weightThresholds.catPresent = values[0];
        weightThresholds.litterDirty = values[1];
        
        if (valueIndex >= 4) {
            weightThresholds.litterEmpty = values[2];
            weightThresholds.emergencyStop = values[3];
        }
        
        thresholdsConfiguredByRaspberry = true;
        lastConfigUpdate = millis();
        
        Serial.println("{\"config\":\"THRESHOLDS_UPDATED\",\"success\":true,\"thresholds\":" + getCurrentThresholds() + "}");
    } else {
        Serial.println("{\"config\":\"THRESHOLDS_UPDATE_FAILED\",\"success\":false,\"error\":\"INSUFFICIENT_PARAMETERS\"}");
    }
}

void CommandProcessor::sendAllSensorReadings() {
    StaticJsonDocument<512> doc;
    doc["weight"] = sensorManager->getWeightReading();
    doc["proximity"] = sensorManager->getProximityReading();
    doc["temperature"] = sensorManager->getTemperatureReading();
    doc["humidity"] = sensorManager->getHumidityReading();
    doc["timestamp"] = millis();
    
    String output;
    serializeJson(doc, output);
    Serial.println(output);
}

void CommandProcessor::sendCurrentConfig() {
    Serial.println("{\"config\":" + getCurrentThresholds() + ",\"configured_by_raspberry\":" + String(thresholdsConfiguredByRaspberry) + ",\"last_update\":" + String(lastConfigUpdate) + "}");
}

void CommandProcessor::sendLitterboxStatus() {
    StaticJsonDocument<512> doc;
    doc["state"] = litterboxMotor->getCurrentState();
    doc["is_blocked"] = litterboxMotor->isBlocked();
    doc["last_cleaning"] = litterboxMotor->getLastCleaningTime();
    doc["safety_status"] = getSafetyStatus();
    
    String output;
    serializeJson(doc, output);
    Serial.println(output);
}

String CommandProcessor::getCurrentThresholds() {
    StaticJsonDocument<256> doc;
    doc["cat_present"] = weightThresholds.catPresent;
    doc["litter_dirty"] = weightThresholds.litterDirty;
    doc["litter_empty"] = weightThresholds.litterEmpty;
    doc["emergency_stop"] = weightThresholds.emergencyStop;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String CommandProcessor::getSafetyStatus() {
    StaticJsonDocument<256> doc;
    doc["cat_present"] = isCatPresent();
    doc["litter_dirty"] = isLitterDirty();
    doc["emergency_condition"] = isEmergencyCondition();
    doc["safe_to_operate"] = isLitterboxSafeToOperate();
    doc["safe_to_clean"] = isLitterboxSafeToClean();
    
    String output;
    serializeJson(doc, output);
    return output;
}

String CommandProcessor::getValidationErrors() {
    StaticJsonDocument<256> doc;
    JsonArray errors = doc.createNestedArray("errors");
    
    if (isCatPresent()) {
        errors.add("CAT_PRESENT");
    }
    if (isEmergencyCondition()) {
        errors.add("EMERGENCY_CONDITION");
    }
    if (!sensorManager->isOperational()) {
        errors.add("SENSOR_MALFUNCTION");
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

void CommandProcessor::resetToDefaults() {
    initializeDefaultThresholds();
    thresholdsConfiguredByRaspberry = false;
    lastConfigUpdate = 0;
    
    // 🔄 RESET IDENTIFICADORES
    for (int i = 0; i < MAX_SENSORS; i++) {
        sensorIdentifiers[i] = "";
        sensorConfigured[i] = false;
    }
    deviceIdentifier = "";
    deviceConfigured = false;
    
    Serial.println("{\"config\":\"RESET_TO_DEFAULTS\",\"success\":true,\"thresholds\":" + getCurrentThresholds() + "}");
}

void CommandProcessor::initializeDefaultThresholds() {
    weightThresholds.catPresent = 3000;        // 3kg
    weightThresholds.litterDirty = 500;        // 500g adicionales
    weightThresholds.litterEmpty = 100;        // Menos de 100g
    weightThresholds.emergencyStop = 10000;    // 10kg (sobrecarga)
}

// =========================================
// 🛡️ MÉTODOS DE SEGURIDAD
// =========================================

bool CommandProcessor::isCatPresent() {
    return sensorManager->getWeightReading() > weightThresholds.catPresent;
}

bool CommandProcessor::isLitterDirty() {
    // Implementar lógica específica para detectar suciedad
    return sensorManager->getWeightReading() > (weightThresholds.catPresent + weightThresholds.litterDirty);
}

bool CommandProcessor::isEmergencyCondition() {
    return sensorManager->getWeightReading() > weightThresholds.emergencyStop;
}

bool CommandProcessor::isLitterboxSafeToOperate() {
    return !isCatPresent() && !isEmergencyCondition() && sensorManager->isOperational();
}

bool CommandProcessor::isLitterboxSafeToClean() {
    return isLitterboxSafeToOperate() && !isLitterDirty();
}
