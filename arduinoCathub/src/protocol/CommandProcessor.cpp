#include "CommandProcessor.h"
#include <ArduinoJson.h>

CommandProcessor::CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* litter, 
  FeederStepperMotor* feeder, WaterDispenserPump* water) 
    : sensorManager(sensors), litterboxMotor(litter), feederMotor(feeder), waterPump(water),
      initialized(false), feederEnabled(true), litterboxState(1) {
}

bool CommandProcessor::initialize() {
    if (sensorManager == nullptr || litterboxMotor == nullptr || 
        feederMotor == nullptr || waterPump == nullptr) {
        Serial.println("{\"error\":\"COMPONENTS_NOT_INITIALIZED\"}");
        return false;
    }
    
    initialized = true;
    Serial.println("{\"system\":\"COMMAND_PROCESSOR_READY\"}");
    return true;
}

void CommandProcessor::processCommand(String command) {
    command.trim();
    if (command.length() == 0) return;
    
    // ===== COMANDO PING =====
    if (command == "PING") {
        Serial.println("{\"response\":\"PONG\"}");
        return;
    }

    // ===== COMANDO ALL =====
    if (command == "ALL") {
        sendAllDevicesStatus();
        return;
    }
    
    // ===== FORMATO: DeviceID:comando (SOLO LTR1 y FDR1) =====
    if (command.startsWith("LTR1:") || command.startsWith("FDR1:")) {
        processDeviceIDCommand(command);
        return;
    }
    
    // ===== COMANDO DESCONOCIDO =====
    Serial.println("{\"error\":\"UNKNOWN_COMMAND\",\"received\":\"" + command + "\"}");
}

void CommandProcessor::processDeviceIDCommand(String command) {
    int colonIndex = command.indexOf(':');
    if (colonIndex == -1) return;
    
    String deviceID = command.substring(0, colonIndex);
    String action = command.substring(colonIndex + 1);
    
    // ===== ARENERO (LTR1) =====
    if (deviceID == "LTR1") {
        if (action == "all") {
            sendLitterboxStatus();
        }
        else if (action == "1") {
            setLitterboxReady();
        }
        else if (action == "2.1") {
            startNormalCleaning();
        }
        else if (action == "2.2") {
            startDeepCleaning();
        }
        else {
            Serial.println("{\"device_id\":\"LTR1\",\"error\":\"INVALID_ACTION\",\"action\":\"" + action + "\",\"valid\":[\"all\",\"1\",\"2.1\",\"2.2\"]}");
        }
    }
    
    // ===== COMEDERO (FDR1) =====
    else if (deviceID == "FDR1") {
        if (action == "all") {
            sendFeederStatus();
        }
        else if (action == "0") {
            disableFeeder();
        }
        else if (action == "1") {
            enableFeeder();
        }
        else {
            Serial.println("{\"device_id\":\"FDR1\",\"error\":\"INVALID_ACTION\",\"action\":\"" + action + "\",\"valid\":[\"all\",\"0\",\"1\"]}");
        }
    }
    
    // ===== DEVICE ID DESCONOCIDO =====
    else {
        Serial.println("{\"error\":\"UNKNOWN_DEVICE_ID\",\"device_id\":\"" + deviceID + "\",\"valid\":[\"LTR1\",\"FDR1\"]}");
    }
}

// ===== IMPLEMENTACIÓN ARENERO (LTR1) =====
void CommandProcessor::sendLitterboxStatus() {
    String response = "{";
    response += "\"device_id\":\"LTR1\",";
    response += "\"type\":\"litterbox\",";
    response += "\"state\":" + String(litterboxState) + ",";
    response += "\"status\":{";
    response += "\"cat_present\":" + String(isCatPresent() ? "true" : "false") + ",";
    response += "\"safe_to_clean\":" + String(isLitterboxSafeToClean() ? "true" : "false") + ",";
    response += "\"distance_cm\":" + String(sensorManager->getLitterboxDistance()) + ",";
    response += "\"temperature_c\":" + String(sensorManager->getLitterboxTemperature()) + ",";
    response += "\"humidity_percent\":" + String(sensorManager->getLitterboxHumidity()) + ",";
    response += "\"gas_ppm\":" + String(sensorManager->getLitterboxGasPPM()) + ",";
    response += "\"motor_ready\":" + String(litterboxMotor->isReady() ? "true" : "false");
    response += "},";
    response += "\"timestamp\":" + String(millis());
    response += "}";
    Serial.println(response);
}

void CommandProcessor::setLitterboxReady() {
    if (isCatPresent()) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"set_ready\",\"success\":false,\"reason\":\"CAT_PRESENT\"}");
        return;
    }
    
    litterboxState = 1; // Estado READY
    litterboxMotor->setState(1);
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"set_ready\",\"success\":true,\"state\":1}");
}

void CommandProcessor::startNormalCleaning() {
    if (!isLitterboxSafeToClean()) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"normal_clean\",\"success\":false,\"reason\":\"NOT_SAFE\"}");
        return;
    }
    
    litterboxState = 21; // Estado 2.1
    bool started = litterboxMotor->executeNormalCleaning();
    
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"normal_clean\",\"success\":" + 
                   String(started ? "true" : "false") + ",\"state\":\"2.1\"}");
}

void CommandProcessor::startDeepCleaning() {
    if (!isLitterboxSafeToClean()) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"deep_clean\",\"success\":false,\"reason\":\"NOT_SAFE\"}");
        return;
    }
    
    litterboxState = 22; // Estado 2.2
    bool started = litterboxMotor->executeDeepCleaning();
    
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"deep_clean\",\"success\":" + 
                   String(started ? "true" : "false") + ",\"state\":\"2.2\"}");
}

// ===== IMPLEMENTACIÓN COMEDERO (FDR1) =====
void CommandProcessor::sendFeederStatus() {
    String response = "{";
    response += "\"device_id\":\"FDR1\",";
    response += "\"type\":\"feeder\",";
    response += "\"enabled\":" + String(feederEnabled ? "true" : "false") + ",";
    response += "\"status\":{";
    response += "\"weight_grams\":" + String(sensorManager->getFeederWeight()) + ",";
    response += "\"cat_distance_cm\":" + String(sensorManager->getFeederCatDistance()) + ",";
    response += "\"food_distance_cm\":" + String(sensorManager->getFeederFoodDistance()) + ",";
    response += "\"cat_eating\":" + String((sensorManager->getFeederCatDistance() <= 15.0) ? "true" : "false") + ",";
    response += "\"needs_refill\":" + String((sensorManager->getFeederWeight() < 150.0) ? "true" : "false") + ",";
    response += "\"has_food\":" + String((sensorManager->getFeederFoodDistance() < 8.0) ? "true" : "false") + ",";
    response += "\"safe_to_operate\":" + String(isFeederSafeToOperate() ? "true" : "false");
    response += "},";
    response += "\"timestamp\":" + String(millis());
    response += "}";
    Serial.println(response);
}

void CommandProcessor::enableFeeder() {
    if (!isFeederSafeToOperate()) {
        Serial.println("{\"device_id\":\"FDR1\",\"action\":\"enable\",\"success\":false,\"reason\":\"CAT_TOO_CLOSE\"}");
        return;
    }
    
    feederEnabled = true;
    feederMotor->enable();
    
    // Dispensar una porción cuando se activa
    bool dispensed = feederMotor->feedPortion(1);
    
    Serial.println("{\"device_id\":\"FDR1\",\"action\":\"enable\",\"success\":true,\"enabled\":true,\"food_dispensed\":" + 
                   String(dispensed ? "true" : "false") + "}");
}

void CommandProcessor::disableFeeder() {
    feederEnabled = false;
    feederMotor->disable();
    Serial.println("{\"device_id\":\"FDR1\",\"action\":\"disable\",\"success\":true,\"enabled\":false}");
}

// ===== VALIDACIONES DE SEGURIDAD =====
bool CommandProcessor::isCatPresent() {
    float distance = sensorManager->getLitterboxDistance();
    return (distance > 0 && distance <= 6.0);
}

bool CommandProcessor::isLitterboxSafeToClean() {
    if (isCatPresent()) return false;
    
    float humidity = sensorManager->getLitterboxHumidity();
    float temperature = sensorManager->getLitterboxTemperature();
    float gasPPM = sensorManager->getLitterboxGasPPM();
    
    return (humidity >= 15.0 && humidity <= 80.0 &&
            temperature >= 10.0 && temperature <= 35.0 &&
            gasPPM <= 800.0);
}

bool CommandProcessor::isFeederSafeToOperate() {
    float catDistance = sensorManager->getFeederCatDistance();
    return (catDistance > 15.0);
}

// ===== COMANDO ALL (SIN WTR1) =====
void CommandProcessor::sendAllDevicesStatus() {
    String response = "{";
    response += "\"command\":\"ALL\",";
    response += "\"timestamp\":" + String(millis()) + ",";
    response += "\"devices\":{";
    
    // ===== ARENERO (LTR1) =====
    response += "\"LTR1\":{";
    response += "\"type\":\"litterbox\",";
    response += "\"state\":" + String(litterboxState) + ",";
    response += "\"sensors\":{";
    response += "\"distance_cm\":" + String(sensorManager->getLitterboxDistance()) + ",";
    response += "\"temperature_c\":" + String(sensorManager->getLitterboxTemperature()) + ",";
    response += "\"humidity_percent\":" + String(sensorManager->getLitterboxHumidity()) + ",";
    response += "\"gas_ppm\":" + String(sensorManager->getLitterboxGasPPM()) + ",";
    response += "\"cat_present\":" + String(isCatPresent() ? "true" : "false") + ",";
    response += "\"safe_to_clean\":" + String(isLitterboxSafeToClean() ? "true" : "false");
    response += "},";
    response += "\"actuators\":{";
    response += "\"motor_ready\":" + String(litterboxMotor->isReady() ? "true" : "false") + ",";
    response += "\"motor_state\":" + String(litterboxMotor->getState());
    response += "}";
    response += "},";
    
    // ===== COMEDERO (FDR1) =====
    response += "\"FDR1\":{";
    response += "\"type\":\"feeder\",";
    response += "\"enabled\":" + String(feederEnabled ? "true" : "false") + ",";
    response += "\"sensors\":{";
    response += "\"weight_grams\":" + String(sensorManager->getFeederWeight()) + ",";
    response += "\"cat_distance_cm\":" + String(sensorManager->getFeederCatDistance()) + ",";
    response += "\"food_distance_cm\":" + String(sensorManager->getFeederFoodDistance()) + ",";
    response += "\"cat_eating\":" + String((sensorManager->getFeederCatDistance() <= 15.0) ? "true" : "false") + ",";
    response += "\"needs_refill\":" + String((sensorManager->getFeederWeight() < 150.0) ? "true" : "false") + ",";
    response += "\"has_food\":" + String((sensorManager->getFeederFoodDistance() < 8.0) ? "true" : "false");
    response += "},";
    response += "\"actuators\":{";
    response += "\"motor_enabled\":" + String(feederMotor->isEnabled() ? "true" : "false") + ",";
    response += "\"can_dispense\":" + String(isFeederSafeToOperate() ? "true" : "false");
    response += "}";
    response += "},";
    
    // ===== BEBEDERO (WTR1) - SOLO INFORMACIÓN, NO COMANDOS =====
    response += "\"WTR1\":{";
    response += "\"type\":\"waterdispenser\",";
    response += "\"mode\":\"automatic\",";
    response += "\"sensors\":{";
    response += "\"water_level\":\"" + sensorManager->getWaterLevel() + "\",";
    response += "\"cat_drinking\":" + String(sensorManager->isCatDrinking() ? "true" : "false") + ",";
    response += "\"ir_detected\":" + String(sensorManager->isWaterIRTriggered() ? "true" : "false");
    response += "},";
    response += "\"actuators\":{";
    response += "\"pump_running\":" + String(waterPump->isPumpRunning() ? "true" : "false") + ",";
    response += "\"auto_control\":true";
    response += "}";
    response += "}";
    
    response += "}}";
    Serial.println(response);
}

// ===== CONTROL AUTOMÁTICO =====
void CommandProcessor::update() {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    
    // Actualizar cada 2 segundos
    if (now - lastUpdate >= 2000) {
        
        // ===== AUTO-RELLENADO DEL COMEDERO =====
        if (feederEnabled && isFeederSafeToOperate()) {
            float weight = sensorManager->getFeederWeight();
            float foodDistance = sensorManager->getFeederFoodDistance();
            
            if (weight < 150.0 && foodDistance < 8.0) {
                feederMotor->feedPortion(1);
                Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"weight\":" + String(weight) + "}");
            }
        }
                // ===== 🔥 AUTO-CONTROL DEL BEBEDERO (COMPLETAMENTE AUTOMÁTICO) =====
                String waterLevel = sensorManager->getWaterLevel();
                bool catNearWater = sensorManager->isCatDrinking();
                
                // 🔥 ACTIVAR BOMBA: Si agua NO está al máximo Y no hay gato
                if (waterLevel != "FLOOD" && !catNearWater && !waterPump->isPumpRunning()) {
                    waterPump->turnOn(30000); // 30 segundos máximo (se parará cuando llegue a FLOOD)
                    Serial.println("{\"auto_action\":\"WATER_PUMP_STARTED\",\"level\":\"" + waterLevel + "\",\"reason\":\"REFILL_NEEDED\"}");
                }
                
                // 🔥 DETENER BOMBA: Si detecta gato (EMERGENCIA)
                if (catNearWater && waterPump->isPumpRunning()) {
                    waterPump->turnOff();
                    Serial.println("{\"auto_action\":\"WATER_PUMP_EMERGENCY_STOP\",\"reason\":\"CAT_DETECTED\"}");
                }
                
                // 🔥 DETENER BOMBA: Si llegó al máximo nivel
                if (waterLevel == "FLOOD" && waterPump->isPumpRunning()) {
                    waterPump->turnOff();
                    Serial.println("{\"auto_action\":\"WATER_PUMP_STOPPED\",\"reason\":\"WATER_LEVEL_FULL\",\"level\":\"FLOOD\"}");
                }
        
        // ===== MONITOREO DEL ARENERO =====
        if (litterboxState == 1 && isCatPresent()) {
            litterboxMotor->setState(-1); // BLOCKED
            Serial.println("{\"safety_alert\":\"LITTERBOX_BLOCKED_CAT_PRESENT\"}");
        }
        
        lastUpdate = now;
    }
}