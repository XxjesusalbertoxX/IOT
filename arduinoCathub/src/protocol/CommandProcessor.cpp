// CommandProcessor.cpp
#include "CommandProcessor.h"
#include <ArduinoJson.h>

CommandProcessor::CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* litter,
    FeederStepperMotor* feeder, WaterDispenserPump* water)
    : sensorManager(sensors),
      litterboxMotor(litter),
      feederMotor(feeder),
      waterPump(water),
      initialized(false),
      manualFeederControl(false),
      litterboxState(1) {
}

bool CommandProcessor::initialize() {
    if (!sensorManager || !litterboxMotor || !feederMotor || !waterPump) {
        Serial.println("{\"command_processor\":\"INITIALIZE_FAILED\",\"reason\":\"NULL_DEPENDENCY\"}");
        return false;
    }

    initialized = true;
    Serial.println("{\"command_processor\":\"INITIALIZED\"}");
    return true;
}

void CommandProcessor::processCommand(String command) {
    command.trim();
    if (command.length() == 0) return;

    if (command == "PING") { Serial.println("{\"response\":\"PONG\"}"); return; }
    if (command == "ALL")    { sendAllDevicesStatus(); return; }

    if (command == "FDR1:1" || command == "FDR1:0") {
        bool active = (command.charAt(5) == '1');
        controlFeederMotor(active);
        return;
    }

    if (command.startsWith("LTR1:")) {
        processDeviceIDCommand(command);
        return;
    }

    Serial.println("{\"error\":\"UNKNOWN_COMMAND\",\"received\":\"" + command + "\"}");
}

void CommandProcessor::processDeviceIDCommand(String command) {
    String deviceId = command.substring(0, 4);
    String action = command.substring(5);

    if (deviceId != "LTR1") {
        Serial.println("{\"device_id\":\"" + deviceId + "\",\"error\":\"UNKNOWN_DEVICE\"}");
        return;
    }

    if (action == "STATUS") {
        sendLitterboxStatus();
    } else if (action == "READY" || action == "2") {
        setLitterboxReady();
    } else if (action == "CLEAN_NORMAL" || action == "2.1") {
        startNormalCleaning();
    } else if (action == "CLEAN_DEEP" || action == "2.2") {
        startDeepCleaning();
    } else {
        Serial.println("{\"device_id\":\"" + deviceId + "\",\"error\":\"UNKNOWN_ACTION\",\"action\":\"" + action + "\"}");
    }
}

// ===== IMPLEMENTACIÓN ARENERO (LTR1) =====
void CommandProcessor::sendLitterboxStatus() {
    int motorState = (litterboxMotor ? litterboxMotor->getState() : 0);
    String stateStr = (motorState == 2) ? "ACTIVE" : (motorState == 1 ? "INACTIVE" : "UNKNOWN");

    String response = "{\"device_id\":\"LTR1\",\"status\":\"" + stateStr + "\",";
    response += "\"state\":" + String(litterboxState) + ",";
    response += "\"distance_cm\":" + String(sensorManager ? sensorManager->getLitterboxDistance() : -1.0) + ",";
    response += "\"temperature_c\":" + String(sensorManager ? sensorManager->getLitterboxTemperature() : -999.0) + ",";
    response += "\"humidity_percent\":" + String(sensorManager ? sensorManager->getLitterboxHumidity() : -1.0) + ",";
    response += "\"gas_ppm\":" + String(sensorManager ? sensorManager->getLitterboxGasPPM() : -1.0) + ",";
    response += "\"motor_ready\":" + String(litterboxMotor ? (litterboxMotor->isReady() ? "true" : "false") : "false") + ",";
    response += "\"safe_to_operate\":" + String(isLitterboxSafeToOperate() ? "true" : "false");
    response += "}";
    Serial.println(response);
}

void CommandProcessor::setLitterboxReady() {
    if (!sensorManager) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"SET_READY\",\"success\":false,\"reason\":\"NO_SENSOR_MANAGER\"}");
        return;
    }
    // if (!isLitterboxSafeToOperate()) {
    //     Serial.println("{\"device_id\":\"LTR1\",\"action\":\"SET_READY\",\"success\":false,\"reason\":\"NOT_SAFE\"}");
    //     return;
    // }
    if (!litterboxMotor) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"SET_READY\",\"success\":false,\"reason\":\"NO_MOTOR\"}");
        return;
    }

    if (litterboxMotor->setReady()) {
        litterboxState = 2;
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"SET_READY\",\"success\":true,\"state\":2}");
    } else {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"SET_READY\",\"success\":false,\"reason\":\"MOTOR_FAILED\"}");
    }
}
void CommandProcessor::startNormalCleaning() {
    if (!litterboxMotor) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"CLEAN_NORMAL\",\"success\":false,\"reason\":\"NO_MOTOR\"}");
        return;
    }
    // if (!isLitterboxSafeToClean()) {
    //     Serial.println("{\"device_id\":\"LTR1\",\"action\":\"CLEAN_NORMAL\",\"success\":false,\"reason\":\"NOT_SAFE\"}");
    //     return;
    // }

    // temporal
    litterboxState = 21;
    bool ok = litterboxMotor->executeNormalCleaning();
    litterboxState = litterboxMotor->getState(); // normalmente 2
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"CLEAN_NORMAL\",\"success\":" + String(ok ? "true" : "false") + ",\"state\":" + String(litterboxState) + "}");
}

void CommandProcessor::startDeepCleaning() {
    if (!litterboxMotor) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"CLEAN_DEEP\",\"success\":false,\"reason\":\"NO_MOTOR\"}");
        return;
    }
    // if (!isLitterboxSafeToClean()) {
    //     Serial.println("{\"device_id\":\"LTR1\",\"action\":\"CLEAN_DEEP\",\"success\":false,\"reason\":\"NOT_SAFE\"}");
    //     return;
    // }

    litterboxState = 22;
    bool ok = litterboxMotor->executeDeepCleaning();
    litterboxState = litterboxMotor->getState(); // debe quedar 1 (INACTIVE)
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"CLEAN_DEEP\",\"success\":" + String(ok ? "true" : "false") + ",\"final_state\":" + String(litterboxState) + "}");
}

// ===== IMPLEMENTACIÓN COMEDERO (FDR1) =====
void CommandProcessor::sendFeederStatus() {
    String response = "{\"device_id\":\"FDR1\",\"status\":\"ACTIVE\",";
    response += "\"manual_control\":" + String(manualFeederControl ? "true" : "false") + ",";
    response += "\"motor_running\":" + String((feederMotor ? feederMotor->isRunning() : false) ? "true" : "false") + ",";
    response += "\"weight_grams\":" + String(sensorManager ? sensorManager->getFeederWeight() : 0.0) + ",";
    response += "\"cat_distance_cm\":" + String(sensorManager ? sensorManager->getFeederCatDistance() : -1.0) + ",";
    response += "\"food_distance_cm\":" + String(sensorManager ? sensorManager->getFeederFoodDistance() : -1.0) + ",";
    response += "\"storage_status\":\"" + String(sensorManager ? sensorManager->getStorageFoodStatus() : "NOT_READY") + "\",";
    response += "\"plate_status\":\"" + String(sensorManager ? sensorManager->getPlateFoodStatus() : "NOT_READY") + "\",";
    response += "\"motor_ready\":" + String((feederMotor ? feederMotor->isReady() : false) ? "true" : "false") + ",";
    response += "\"safe_to_operate\":" + String(isFeederSafeToOperate() ? "true" : "false");
    response += "}";
    Serial.println(response);
}

void CommandProcessor::controlFeederMotor(bool on) {
    // El front envía FDR1:1 para presionar (persistent), FDR1:0 para soltar.
    manualFeederControl = on;

    if (on) {
        if (!sensorManager || !feederMotor) {
            Serial.println("{\"device_id\":\"FDR1\",\"action\":\"manual_control\",\"success\":false,\"reason\":\"MISSING_DEPENDENCY\"}");
            manualFeederControl = false;
            return;
        }

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
        if (feederMotor) feederMotor->emergencyStop();
        manualFeederControl = false;
        Serial.println("{\"device_id\":\"FDR1\",\"action\":\"manual_control\",\"success\":true,\"motor\":\"OFF\"}");
    }
}

// ===== VALIDACIONES DE SEGURIDAD =====
bool CommandProcessor::isCatPresent() {
    if (!sensorManager) return false;
    float feederDist = sensorManager->getFeederCatDistance();
    float litterDist = sensorManager->getLitterboxDistance();
    bool feederDetect = (feederDist > 0.0f && feederDist < 10.0f);
    bool litterDetect = (litterDist > 0.0f && litterDist < 15.0f);
    return feederDetect || litterDetect;
}

bool CommandProcessor::isLitterboxSafeToClean() {
    if (!sensorManager) return false;
    float ppm = sensorManager->getLitterboxGasPPM();
    bool gasOk = (ppm >= 0.0f && ppm < 100.0f); // ajusta umbral
    return !isCatPresent() && gasOk;
}

bool CommandProcessor::isLitterboxSafeToOperate() {
    if (!sensorManager) return false;
    float ppm = sensorManager->getLitterboxGasPPM();
    bool gasOk = (ppm >= 0.0f && ppm < 150.0f); // ajusta umbral
    return !isCatPresent() && gasOk;
}

bool CommandProcessor::isFeederSafeToOperate() {
    if (sensorManager) {
        if (hasSufficientFood()) {
            return true;
        }
    }
    return false;
}

bool CommandProcessor::hasSufficientFood() {
    if (!sensorManager) return false;
    float foodDistance = sensorManager->getFeederFoodDistance();
    return (foodDistance > 0 && foodDistance < 13.0);
}

// ===== COMANDO ALL =====
void CommandProcessor::sendAllDevicesStatus() {
    String response = "{\"command\":\"ALL\",\"devices\":{";
    response += "\"LTR1\":{\"state\":" + String(litterboxState) + ",\"safe\":" + String(isLitterboxSafeToOperate() ? "true":"false") + "},";
    response += "\"FDR1\":{},\"WTR1\":{}";
    response += "}}";
    Serial.println(response);
}

// ===== CONTROL AUTOMÁTICO =====
void CommandProcessor::update() {
    static unsigned long lastUpdate = 0;
    unsigned long now = millis();

    if (now - lastUpdate >= 500) {
        // FEEDER: control persistente (manualFeederControl)
        if (manualFeederControl && sensorManager && feederMotor) {
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

        // WATER: control automático
        if (sensorManager && waterPump) {
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
        }

        if (litterboxMotor && sensorManager) {
            int motorState = litterboxMotor->getState();
            // Solo monitoreo de seguridad, sin limpieza automática
            if (motorState == 2 && !isLitterboxSafeToOperate()) {
                Serial.println("{\"safety_alert\":\"LITTERBOX_BLOCKED\",\"reason\":\"UNSAFE_CONDITIONS\"}");
                // No llamar a setBlocked() si no existe
            }
        }

        lastUpdate = now;
    }
}
