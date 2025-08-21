// CommandProcessor.cpp
#include "CommandProcessor.h"
#include <ArduinoJson.h>

CommandProcessor::CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* litter, 
  FeederStepperMotor* feeder, WaterDispenserPump* water) 
    : sensorManager(sensors), litterboxMotor(litter), feederMotor(feeder), waterPump(water),
      initialized(false), manualFeederControl(false), litterboxState(1) {
}

bool CommandProcessor::initialize() {
    if (!sensorManager || !litterboxMotor || !feederMotor || !waterPump) {
        return false;
    }
    
    initialized = true;
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

    // ===== COMANDOS DE AGUA =====
    if (command == "WATER_ON") {
        waterPump->turnOn(120000);
        Serial.println("{\"manual_command\":\"WATER_PUMP_ON\",\"duration_ms\":120000,\"duration_min\":2}");
        return;
    }
    
    if (command == "WATER_OFF") {
        waterPump->turnOff();
        Serial.println("{\"manual_command\":\"WATER_PUMP_OFF\"}");
        return;
    }
    
    if (command == "WATER_STATUS") {
        String response = "{\"manual_command\":\"WATER_STATUS\",";
        response += "\"water_level\":\"" + sensorManager->getWaterLevel() + "\",";
        response += "\"analog_value\":" + String(sensorManager->getWaterSensor()->getAnalogValue()) + ",";
        response += "\"cat_drinking\":" + String(sensorManager->isCatDrinking() ? "true" : "false") + ",";
        response += "\"pump_running\":" + String(waterPump->isPumpRunning() ? "true" : "false") + ",";
        response += "\"remaining_time_ms\":" + String(waterPump->getRemainingTime()) + "}";
        Serial.println(response);
        return;
    }
    
    // 🔥 COMANDOS DEL MOTOR: FDR1:1 y FDR1:0
    if (command == "FDR1:1" || command == "FDR1:0") {
        bool active = (command.charAt(5) == '1');
        controlFeederMotor(active);
        return;
    }
    
    // ===== FORMATO: DeviceID:comando (SOLO LTR1) =====
    if (command.startsWith("LTR1:")) {
        processDeviceIDCommand(command);
        return;
    }
    
    // ===== COMANDO DESCONOCIDO =====
    Serial.println("{\"error\":\"UNKNOWN_COMMAND\",\"received\":\"" + command + "\"}");
}

void CommandProcessor::processDeviceIDCommand(String command) {
    String deviceId = command.substring(0, 4);
    String action = command.substring(5);
    
    if (deviceId == "LTR1") {
        if (action == "STATUS") {
            sendLitterboxStatus();
        } else if (action == "READY") {
            setLitterboxReady();
        } else if (action == "CLEAN_NORMAL") {
            startNormalCleaning();
        } else if (action == "CLEAN_DEEP") {
            startDeepCleaning();
        } else if (action.startsWith("INTERVAL:")) {
            int minutes = action.substring(9).toInt();
            setLitterboxCleaningInterval(minutes);
        } else {
            Serial.println("{\"device_id\":\"" + deviceId + "\",\"error\":\"UNKNOWN_ACTION\",\"action\":\"" + action + "\"}");
        }
    }
}

// ===== IMPLEMENTACIÓN ARENERO (LTR1) =====
void CommandProcessor::sendLitterboxStatus() {
    String response = "{\"device_id\":\"LTR1\",\"status\":\"ACTIVE\",";
    response += "\"state\":" + String(litterboxState) + ",";
    response += "\"distance_cm\":" + String(sensorManager->getLitterboxDistance()) + ",";
    response += "\"temperature_c\":" + String(sensorManager->getLitterboxTemperature()) + ",";
    response += "\"humidity_percent\":" + String(sensorManager->getLitterboxHumidity()) + ",";
    response += "\"gas_ppm\":" + String(sensorManager->getLitterboxGasPPM()) + ",";
    response += "\"motor_ready\":" + String(litterboxMotor->isReady() ? "true" : "false") + ",";
    response += "\"safe_to_operate\":" + String(isLitterboxSafeToOperate() ? "true" : "false");
    response += "}";
    Serial.println(response);
}

void CommandProcessor::setLitterboxReady() {
    litterboxState = 2;
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"SET_READY\",\"success\":true,\"state\":2}");
}

void CommandProcessor::startNormalCleaning() {
    if (!isLitterboxSafeToClean()) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"CLEAN_NORMAL\",\"success\":false,\"reason\":\"NOT_SAFE\"}");
        return;
    }
    
    litterboxState = 21;
    litterboxMotor->executeNormalCleaning(); // ✅ CHANGE FROM startNormalCleaning
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"CLEAN_NORMAL\",\"success\":true,\"state\":2.1}");
}

void CommandProcessor::startDeepCleaning() {
    if (!isLitterboxSafeToClean()) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"CLEAN_DEEP\",\"success\":false,\"reason\":\"NOT_SAFE\"}");
        return;
    }
    
    litterboxState = 22;
    litterboxMotor->executeDeepCleaning(); // ✅ CHANGE FROM startDeepCleaning
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"CLEAN_DEEP\",\"success\":true,\"state\":2.2}");
}

void CommandProcessor::setLitterboxCleaningInterval(int minutes) {
    litterboxMotor->setCleaningInterval(minutes);
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"SET_INTERVAL\",\"success\":true,\"interval_minutes\":" + String(minutes) + "}");
}

bool CommandProcessor::isLitterboxSafeToOperate() {
    return !isCatPresent() && (sensorManager->getLitterboxGasPPM() < 150.0);
}

// ===== IMPLEMENTACIÓN COMEDERO (FDR1) =====
void CommandProcessor::sendFeederStatus() {
    String response = "{\"device_id\":\"FDR1\",\"status\":\"ACTIVE\",";
    // 🔥 ELIMINAR REFERENCIAS A feederEnabled y targetWeight
    response += "\"manual_control\":" + String(manualFeederControl ? "true" : "false") + ",";
    response += "\"motor_running\":" + String(feederMotor->isRunning() ? "true" : "false") + ",";
    response += "\"weight_grams\":" + String(sensorManager->getFeederWeight()) + ",";
    response += "\"cat_distance_cm\":" + String(sensorManager->getFeederCatDistance()) + ",";
    response += "\"food_distance_cm\":" + String(sensorManager->getFeederFoodDistance()) + ",";
    response += "\"storage_status\":\"" + sensorManager->getStorageFoodStatus() + "\",";
    response += "\"plate_status\":\"" + sensorManager->getPlateFoodStatus() + "\",";
    response += "\"motor_ready\":" + String(feederMotor->isReady() ? "true" : "false") + ",";
    response += "\"safe_to_operate\":" + String(isFeederSafeToOperate() ? "true" : "false");
    response += "}";
    Serial.println(response);
}

// 🔥 ELIMINAR ESTE MÉTODO QUE NO EXISTE EN EL .h:
// void CommandProcessor::setTargetWeight(int grams) - NO NECESARIO

void CommandProcessor::controlFeederMotor(bool on) {
    // El front envía FDR1:1 para presionar (persistent), FDR1:0 para soltar.
    manualFeederControl = on;

    if (on) {
        float storageDistance = sensorManager->getFeederFoodDistance();
        float plateDistance = sensorManager->getFeederCatDistance();

        // Intentar arrancar usando tryStart (valida sensores y arranca si todo ok).
        bool started = feederMotor->tryStart(storageDistance, plateDistance);
        if (!started) {
            // Si no pudo arrancar, no dejamos persistencia.
            manualFeederControl = false;

            String reason = "SENSOR_CHECK_FAILED";
            if (storageDistance <= 0 || storageDistance >= 13.0) {
                reason = "NO_FOOD_IN_STORAGE";
            } else if (plateDistance > 0 && plateDistance <= 2.0) {
                reason = "PLATE_ALREADY_FULL";
            }

            Serial.println("{\"device_id\":\"FDR1\",\"action\":\"manual_control\",\"success\":false,\"reason\":\"" + reason + "\",\"storage_distance\":" + String(storageDistance) + ",\"plate_distance\":" + String(plateDistance) + "}");
            return;
        }

        // Si arranca, dejamos manualFeederControl = true (persistente hasta que se suelte o validación lo detenga)
        Serial.println("{\"device_id\":\"FDR1\",\"action\":\"manual_control\",\"success\":true,\"motor\":\"ON\",\"direction\":\"LEFT\",\"speed\":120}");
    } else {
        // Cuando sueltan el botón, parar inmediatamente.
        feederMotor->emergencyStop();
        manualFeederControl = false;
        Serial.println("{\"device_id\":\"FDR1\",\"action\":\"manual_control\",\"success\":true,\"motor\":\"OFF\"}");
    }
}

// ===== VALIDACIONES DE SEGURIDAD =====
bool CommandProcessor::isCatPresent() {
    return (sensorManager->getFeederCatDistance() < 10.0) || (sensorManager->getLitterboxDistance() < 15.0);
}

bool CommandProcessor::isLitterboxSafeToClean() {
    return !isCatPresent() && (sensorManager->getLitterboxGasPPM() < 100.0);
}

bool CommandProcessor::isFeederSafeToOperate() {
    return hasSufficientFood() && !isCatPresent();
}

// 🔥 DEJAR SOLO UNA DEFINICIÓN DE hasSufficientFood():
bool CommandProcessor::hasSufficientFood() {
    float foodDistance = sensorManager->getFeederFoodDistance();
    // Consideramos que >=13.0 cm = vacío; <=0 = no lectura -> no suficiente
    // Cualquier lectura válida < 13.0 se considera que hay comida (aunque quizás parcial)
    return (foodDistance > 0 && foodDistance < 13.0);
}



// ===== COMANDO ALL =====
void CommandProcessor::sendAllDevicesStatus() {
    String response = "{\"command\":\"ALL\",\"devices\":{";
    
    // ===== ARENERO =====
    response += "\"LTR1\":{";
    response += "\"state\":" + String(litterboxState) + ",";
    response += "\"distance_cm\":" + String(sensorManager->getLitterboxDistance()) + ",";
    response += "\"temperature_c\":" + String(sensorManager->getLitterboxTemperature()) + ",";
    response += "\"humidity_percent\":" + String(sensorManager->getLitterboxHumidity()) + ",";
    response += "\"gas_ppm\":" + String(sensorManager->getLitterboxGasPPM()) + ",";
    response += "\"safe_to_operate\":" + String(isLitterboxSafeToOperate() ? "true" : "false");
    response += "},";
    
    // ===== COMEDERO =====
    response += "\"FDR1\":{";
    // 🔥 ELIMINAR REFERENCIAS A feederEnabled y targetWeight
    response += "\"manual_control\":" + String(manualFeederControl ? "true" : "false") + ",";
    response += "\"motor_running\":" + String(feederMotor->isRunning() ? "true" : "false") + ",";
    response += "\"weight_grams\":" + String(sensorManager->getFeederWeight()) + ",";
    response += "\"cat_distance_cm\":" + String(sensorManager->getFeederCatDistance()) + ",";
    response += "\"food_distance_cm\":" + String(sensorManager->getFeederFoodDistance()) + ",";
    response += "\"storage_status\":\"" + sensorManager->getStorageFoodStatus() + "\",";
    response += "\"plate_status\":\"" + sensorManager->getPlateFoodStatus() + "\",";
    response += "\"safe_to_operate\":" + String(isFeederSafeToOperate() ? "true" : "false");
    response += "},";
    
    // ===== BEBEDERO =====
    response += "\"WTR1\":{";
    response += "\"water_level\":\"" + sensorManager->getWaterLevel() + "\",";
    response += "\"cat_drinking\":" + String(sensorManager->isCatDrinking() ? "true" : "false") + ",";
    response += "\"pump_running\":" + String(waterPump->isPumpRunning() ? "true" : "false") + ",";
    response += "\"remaining_time_ms\":" + String(waterPump->getRemainingTime());
    response += "}";
    
    response += "}}";
    Serial.println(response);
}

// ===== CONTROL AUTOMÁTICO =====
void CommandProcessor::update() {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();
    
    if (now - lastUpdate >= 500) {
        
        // 🔥 PARADA AUTOMÁTICA POR SEGURIDAD DEL COMEDERO
        // ===== PARADA/AUTOMATISMO DEL COMEDERO (persistente desde frontend) =====
        // ===== PARADA/AUTOMATISMO DEL COMEDERO (persistente desde frontend) =====
        if (manualFeederControl) {
            float storageDistance = sensorManager->getFeederFoodDistance();
            float plateDistance = sensorManager->getFeederCatDistance();

            // Si el motor no está corriendo, intentar arrancar (persistente)
            if (!feederMotor->isRunning()) {
                bool started = feederMotor->tryStart(storageDistance, plateDistance);
                if (!started) {
                    // Si no pudo arrancar por sensores, cancelamos la persistencia
                    manualFeederControl = false;
                    String reason = "SENSOR_CHECK_FAILED";
                    if (storageDistance <= 0 || storageDistance >= 13.0) {
                        reason = "NO_FOOD_IN_STORAGE";
                    } else if (plateDistance > 0 && plateDistance <= 2.0) {
                        reason = "PLATE_FULL";
                    }
                    Serial.println("{\"auto_action\":\"FEEDER_START_BLOCKED\",\"reason\":\"" + reason + "\",\"storage_distance\":" + String(storageDistance) + ",\"plate_distance\":" + String(plateDistance) + "}");
                }
            } else {
                // Si ya está corriendo, verificar que siga siendo seguro; si no, detener inmediatamente
                if (feederMotor->monitorAndStop(storageDistance, plateDistance)) {
                    // monitorAndStop detuvo el motor por razones de seguridad -> cancelamos persistencia
                    manualFeederControl = false;
                    Serial.println("{\"auto_action\":\"FEEDER_AUTO_STOPPED_BY_SENSORS\",\"storage_distance\":" + String(storageDistance) + ",\"plate_distance\":" + String(plateDistance) + "}");
                }
            }
        }
        // ===== AUTO-CONTROL DEL BEBEDERO =====
        String waterLevel = sensorManager->getWaterLevel();
        bool catNearWater = sensorManager->isCatDrinking();
        
        if (waterLevel != "FLOOD" && !catNearWater && !waterPump->isPumpRunning()) {
            waterPump->turnOn(30000);
            Serial.println("{\"auto_action\":\"WATER_PUMP_STARTED\",\"level\":\"" + waterLevel + "\",\"reason\":\"REFILL_NEEDED\"}");
        }
        
        if (catNearWater && waterPump->isPumpRunning()) {
            waterPump->turnOff();
            Serial.println("{\"auto_action\":\"WATER_PUMP_EMERGENCY_STOP\",\"reason\":\"CAT_DETECTED\"}");
        }
        
        if (waterLevel == "FLOOD" && waterPump->isPumpRunning()) {
            waterPump->turnOff();
            Serial.println("{\"auto_action\":\"WATER_PUMP_STOPPED\",\"reason\":\"WATER_LEVEL_FULL\",\"level\":\"FLOOD\"}");
        }
        
        // ===== MONITOREO DEL ARENERO =====
        if (litterboxMotor->getState() == 2 && litterboxMotor->shouldPerformCleaning() && isLitterboxSafeToOperate()) {
            Serial.println("{\"auto_action\":\"LITTERBOX_SCHEDULED_CLEANING\",\"reason\":\"INTERVAL_REACHED\"}");
            startNormalCleaning();
        }
        
        if (litterboxMotor->getState() > 0 && !isLitterboxSafeToOperate()) {
            litterboxMotor->setBlocked();
            litterboxState = -1;
            Serial.println("{\"safety_alert\":\"LITTERBOX_BLOCKED\",\"reason\":\"" + 
                           String(isCatPresent() ? "CAT_PRESENT" : "HIGH_GAS_LEVEL") + "\"}");
        }
        
        lastUpdate = now;
    }
}