// CommandProcessor.cpp
#include "CommandProcessor.h"
#include <ArduinoJson.h>

CommandProcessor::CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* litter, 
  FeederStepperMotor* feeder, WaterDispenserPump* water) 
    : sensorManager(sensors), litterboxMotor(litter), feederMotor(feeder), waterPump(water),
      initialized(false), feederEnabled(true), targetWeight(400), manualFeederControl(false),
      lastFeederRetry(0), litterboxState(1),
      feederRefillInProgress(false), feederRefillStartTime(0), feederRefillStartWeight(0.0f) {  // 🔥 Inicializar nuevas variables
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

    if (command == "WATER_ON") {
        waterPump->turnOn(120000);  // 2 minutos = 120,000 ms
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
    
    // ===== FORMATO: FDR1:WIT_001:{gramos} =====
    if (command.startsWith("FDR1:WIT_001:")) {
        String weightStr = command.substring(12);
        int weight = weightStr.toInt();
        if (weight > 0) {
            setTargetWeight(weight);
        } else {
            Serial.println("{\"device_id\":\"FDR1\",\"error\":\"INVALID_WEIGHT\",\"received\":\"" + weightStr + "\"}");
        }
        return;
    }
    
    // ===== NUEVO FORMATO: FDR1:WIT_001:{gramos} =====
    if (command.startsWith("FDR1:WIT_001:")) {
        String weightStr = command.substring(12); // Extrae después de "FDR1:WIT_001:"
        int weight = weightStr.toInt();
        if (weight > 0) {
            setTargetWeight(weight);
        } else {
            Serial.println("{\"device_id\":\"FDR1\",\"error\":\"INVALID_WEIGHT\",\"received\":\"" + weightStr + "\"}");
        }
        return;
    }
    
    // ===== NUEVO FORMATO: FDR1:{0/1} =====
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
int colonIndex = command.indexOf(':');
    if (colonIndex == -1) return;
    
    String deviceID = command.substring(0, colonIndex);
    String action = command.substring(colonIndex + 1);
    
    // ===== ARENERO (LTR1) =====
    if (deviceID == "LTR1") {
        if (action == "all") {
            sendLitterboxStatus();
        }
        else if (action == "2") {
            setLitterboxReady();
        }
        else if (action == "2.1") {
            startNormalCleaning();
        }
        else if (action == "2.2") {
            startDeepCleaning();
        }
        else if (action.startsWith("LTMTR_001:")) {
            String intervalStr = action.substring(10); // Extrae después de "LTMTR_001:"
            int minutes = intervalStr.toInt();
            if (minutes > 0) {
                setLitterboxCleaningInterval(minutes);
            } else {
                Serial.println("{\"device_id\":\"LTR1\",\"error\":\"INVALID_INTERVAL\",\"received\":\"" + intervalStr + "\"}");
            }
        }
        else {
            Serial.println("{\"device_id\":\"LTR1\",\"error\":\"INVALID_ACTION\",\"action\":\"" + action + "\",\"valid\":[\"all\",\"2\",\"2.1\",\"2.2\",\"LTMTR_001:{minutes}\"]}");
        }
    }
    // ===== DEVICE ID DESCONOCIDO =====
    else {
        Serial.println("{\"error\":\"UNKNOWN_DEVICE_ID\",\"device_id\":\"" + deviceID + "\",\"valid\":[\"LTR1\",\"FDR1\"]}");
    }
}

// ===== IMPLEMENTACIÓN ARENERO (LTR1) =====
void CommandProcessor::sendLitterboxStatus() {
    int currentState = litterboxMotor->getState();
    
    String response = "{";
    response += "\"device_id\":\"LTR1\",";
    response += "\"type\":\"litterbox\",";
    response += "\"state\":" + String(currentState) + ",";
    response += "\"status\":{";
    response += "\"cat_present\":" + String(isCatPresent() ? "true" : "false") + ",";
    response += "\"safe_to_operate\":" + String(isLitterboxSafeToOperate() ? "true" : "false") + ",";
    response += "\"distance_cm\":" + String(sensorManager->getLitterboxDistance()) + ",";
    response += "\"temperature_c\":" + String(sensorManager->getLitterboxTemperature()) + ",";
    response += "\"humidity_percent\":" + String(sensorManager->getLitterboxHumidity()) + ",";
    response += "\"gas_ppm\":" + String(sensorManager->getLitterboxGasPPM()) + ",";
    response += "\"torque_active\":" + String(litterboxMotor->isTorqueActive() ? "true" : "false") + ",";
    response += "\"motor_position\":" + String(litterboxMotor->getCurrentPosition()) + ",";
    response += "\"cleaning_interval_min\":" + String(litterboxMotor->getCleaningInterval()) + ",";
    response += "\"last_cleaned_ms\":" + String(litterboxMotor->getLastCleaningTime());
    response += "},";
    response += "\"timestamp\":" + String(millis());
    response += "}";
    Serial.println(response);
}

void CommandProcessor::setLitterboxReady() {
    // Solo se puede activar desde estado 1
    if (litterboxMotor->getState() != 1) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"set_ready\",\"success\":false,\"reason\":\"INVALID_STATE_TRANSITION\"}");
        return;
    }
    
    // Verificar condiciones de seguridad
    if (!isLitterboxSafeToOperate()) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"set_ready\",\"success\":false,\"reason\":\"UNSAFE_CONDITIONS\"}");
        return;
    }
    
    bool success = litterboxMotor->setReady();
    litterboxState = 2; // Actualizar variable de estado global
    
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"set_ready\",\"success\":" + 
                   String(success ? "true" : "false") + ",\"state\":2}");
}

void CommandProcessor::startNormalCleaning() {
    // Solo se puede ejecutar desde estado 2 (ACTIVE)
    if (litterboxMotor->getState() != 2) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"normal_clean\",\"success\":false,\"reason\":\"NOT_IN_READY_STATE\"}");
        return;
    }
    
    // Verificar condiciones de seguridad
    if (!isLitterboxSafeToOperate()) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"normal_clean\",\"success\":false,\"reason\":\"UNSAFE_CONDITIONS\"}");
        return;
    }
    
    bool success = litterboxMotor->executeNormalCleaning();
    
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"normal_clean\",\"success\":" + 
                   String(success ? "true" : "false") + ",\"state\":\"2.1\"}");
}

void CommandProcessor::startDeepCleaning() {
    // Solo se puede ejecutar desde estado 2 (ACTIVE)
    if (litterboxMotor->getState() != 2) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"deep_clean\",\"success\":false,\"reason\":\"NOT_IN_READY_STATE\"}");
        return;
    }
    
    // Verificar condiciones de seguridad
    if (!isLitterboxSafeToOperate()) {
        Serial.println("{\"device_id\":\"LTR1\",\"action\":\"deep_clean\",\"success\":false,\"reason\":\"UNSAFE_CONDITIONS\"}");
        return;
    }
    
    bool success = litterboxMotor->executeDeepCleaning();
    if (success) {
        litterboxState = 1; // Actualizar variable de estado global
    }
    
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"deep_clean\",\"success\":" + 
                   String(success ? "true" : "false") + ",\"new_state\":1}");
}

void CommandProcessor::setLitterboxCleaningInterval(int minutes) {
    if (minutes < 1) minutes = 1;
    if (minutes > 1440) minutes = 1440; // Máximo 24 horas
    
    litterboxMotor->setCleaningInterval(minutes);
    
    Serial.println("{\"device_id\":\"LTR1\",\"action\":\"set_cleaning_interval\",\"success\":true,\"minutes\":" + String(minutes) + "}");
}
// ===== VALIDACIÓN ESPECÍFICA PARA EL ARENERO =====
bool CommandProcessor::isLitterboxSafeToOperate() {
    // 1. Verificar si hay gato dentro
    if (isCatPresent()) return false;
    
    // 2. Verificar nivel de gas (>800 PPM = peligroso)
    float gasPPM = sensorManager->getLitterboxGasPPM();
    if (gasPPM > 800.0) return false;
    
    return true;
}

// ===== IMPLEMENTACIÓN COMEDERO (FDR1) =====
void CommandProcessor::sendFeederStatus() {
    String response = "{";
    response += "\"device_id\":\"FDR1\",";
    response += "\"type\":\"feeder\",";
    response += "\"enabled\":" + String(feederEnabled ? "true" : "false") + ",";
    response += "\"status\":{";
    response += "\"weight_grams\":" + String(sensorManager->getFeederWeight()) + ",";
    response += "\"target_weight_grams\":" + String(targetWeight) + ",";
    response += "\"cat_distance_cm\":" + String(sensorManager->getFeederCatDistance()) + ",";
    response += "\"food_distance_cm\":" + String(sensorManager->getFeederFoodDistance()) + ",";
    response += "\"cat_eating\":" + String((sensorManager->getFeederCatDistance() <= 15.0) ? "true" : "false") + ",";
    response += "\"needs_refill\":" + String((sensorManager->getFeederWeight() < targetWeight) ? "true" : "false") + ",";
    response += "\"has_food_in_storage\":" + String(hasSufficientFood() ? "true" : "false") + ",";
    response += "\"motor_active\":" + String(manualFeederControl || feederMotor->isRunning() ? "true" : "false") + ",";
    response += "\"motor_enabled\":" + String(feederMotor->isEnabled() ? "true" : "false");
    response += "},";
    response += "\"timestamp\":" + String(millis());
    response += "}";
    Serial.println(response);
}

void CommandProcessor::setTargetWeight(int grams) {
    if (grams < 50) grams = 50;   // Mínimo 50g
    if (grams > 1000) grams = 1000; // Máximo 1kg
    
    targetWeight = grams;
    Serial.println("{\"device_id\":\"FDR1\",\"action\":\"set_target_weight\",\"success\":true,\"grams\":" + String(targetWeight) + "}");
    
    // Intentar rellenar ahora mismo si es necesario
    float currentWeight = sensorManager->getFeederWeight();
    if (currentWeight < targetWeight) {
        attemptFeederRefill();
    }
}

void CommandProcessor::controlFeederMotor(bool on) {
    // 🔥 VERIFICAR SI HAY REFILL AUTOMÁTICO EN PROGRESO
    if (feederRefillInProgress && on) {
        Serial.println("{\"device_id\":\"FDR1\",\"action\":\"manual_control\",\"success\":false,\"reason\":\"AUTO_REFILL_IN_PROGRESS\"}");
        return;
    }
    
    // 🔥 SI SE ACTIVA CONTROL MANUAL, CANCELAR REFILL AUTOMÁTICO
    if (on && feederRefillInProgress) {
        feederMotor->stopContinuous();
        feederRefillInProgress = false;
        Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"status\":\"CANCELLED_BY_MANUAL_CONTROL\"}");
    }
    
    manualFeederControl = on;
    
    if (on) {
        // Verificar si es seguro y hay suficiente comida
        if (!isFeederSafeToOperate()) {
            Serial.println("{\"device_id\":\"FDR1\",\"action\":\"manual_control\",\"success\":false,\"reason\":\"CAT_TOO_CLOSE\"}");
            return;
        }
        
        if (!hasSufficientFood()) {
            Serial.println("{\"device_id\":\"FDR1\",\"action\":\"manual_control\",\"success\":false,\"reason\":\"INSUFFICIENT_FOOD_STORAGE\"}");
            return;
        }
        
        // Activar motor a velocidad 50 hacia la izquierda
        feederMotor->enable();
        feederMotor->setDirection(false); // Izquierda
        feederMotor->setSpeed(50);        // Velocidad 50 (manual más lento que auto)
        feederMotor->startContinuous();   // Modo continuo
        
        Serial.println("{\"device_id\":\"FDR1\",\"action\":\"manual_control\",\"success\":true,\"motor\":\"ON\",\"direction\":\"LEFT\",\"speed\":50}");
    } 
    else {
        // Detener motor
        feederMotor->stopContinuous();
        Serial.println("{\"device_id\":\"FDR1\",\"action\":\"manual_control\",\"success\":true,\"motor\":\"OFF\"}");
    }
}

void CommandProcessor::attemptFeederRefill() {
    // ===== VERIFICACIONES PREVIAS =====
    if (feederRefillInProgress) {
        Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"status\":\"ALREADY_IN_PROGRESS\"}");
        return;
    }
    
    if (!isFeederSafeToOperate()) {
        Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"status\":\"ABORTED\",\"reason\":\"CAT_TOO_CLOSE\"}");
        return;
    }
    
    if (!hasSufficientFood()) {
        Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"status\":\"ABORTED\",\"reason\":\"INSUFFICIENT_FOOD_STORAGE\"}");
        return;
    }
    
    float currentWeight = sensorManager->getFeederWeight();
    if (currentWeight >= targetWeight) {
        Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"status\":\"NOT_NEEDED\",\"current_weight\":" + String(currentWeight) + ",\"target_weight\":" + String(targetWeight) + "}");
        return;
    }
    
    // ===== INICIAR PROCESO NO-BLOQUEANTE =====
    feederMotor->enable();
    feederMotor->setDirection(false); // Izquierda para dispensar
    feederMotor->setSpeed(120);       // 🔥 Velocidad ajustada (era 50, ahora 120)
    feederMotor->startContinuous();
    
    // ===== CONFIGURAR VARIABLES DE ESTADO =====
    feederRefillInProgress = true;
    feederRefillStartTime = millis();
    feederRefillStartWeight = currentWeight;
    
    Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"status\":\"STARTED\",\"start_weight\":" + 
                   String(feederRefillStartWeight) + ",\"target_weight\":" + String(targetWeight) + 
                   ",\"speed\":120}");
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
    return (catDistance > 15.0); // Seguro si el gato está lejos (>15cm)
}

bool CommandProcessor::hasSufficientFood() {
    float foodDistance = sensorManager->getFeederFoodDistance();
    return (foodDistance >= 7.0); // Si distancia >= 7cm, hay suficiente comida
}


// ===== COMANDO ALL (Con sección del bebedero automático) =====
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
    
    // ===== COMEDERO (FDR1) - ACTUALIZADO =====
    response += "\"FDR1\":{";
    response += "\"type\":\"feeder\",";
    response += "\"enabled\":" + String(feederEnabled ? "true" : "false") + ",";
    response += "\"sensors\":{";
    response += "\"weight_grams\":" + String(sensorManager->getFeederWeight()) + ",";
    response += "\"target_weight_grams\":" + String(targetWeight) + ",";
    response += "\"cat_distance_cm\":" + String(sensorManager->getFeederCatDistance()) + ",";
    response += "\"food_distance_cm\":" + String(sensorManager->getFeederFoodDistance()) + ",";
    response += "\"cat_eating\":" + String((sensorManager->getFeederCatDistance() <= 15.0) ? "true" : "false") + ",";
    response += "\"needs_refill\":" + String((sensorManager->getFeederWeight() < targetWeight) ? "true" : "false") + ",";
    response += "\"has_food_in_storage\":" + String(hasSufficientFood() ? "true" : "false");
    response += "},";
    response += "\"actuators\":{";
    response += "\"motor_enabled\":" + String(feederMotor->isEnabled() ? "true" : "false") + ",";
    response += "\"motor_active\":" + String(feederMotor->isRunning() ? "true" : "false") + ",";
    response += "\"manual_control\":" + String(manualFeederControl ? "true" : "false");
    response += "}";
    response += "},";
    
    // ===== BEBEDERO (WTR1) - AUTOMÁTICO =====
    response += "\"WTR1\":{";
    response += "\"type\":\"waterdispenser\",";
    response += "\"mode\":\"automatic\",";
    response += "\"sensors\":{";
    response += "\"water_level\":\"" + sensorManager->getWaterLevel() + "\",";
    response += "\"cat_drinking\":" + String(sensorManager->isCatDrinking() ? "true" : "false") + ",";
    response += "\"ir_detected\":" + String(sensorManager->isCatDrinking() ? "true" : "false");
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
    
    // 🔥 ACTUALIZAR CADA 500ms PARA MAYOR REACTIVIDAD (era 2000ms)
    if (now - lastUpdate >= 500) {
        
        // ===== CONTROL DEL PROCESO DE REFILL NO-BLOQUEANTE =====
        if (feederRefillInProgress) {
            float currentWeight = sensorManager->getFeederWeight();
            float weightIncrease = currentWeight - feederRefillStartWeight;
            unsigned long elapsedTime = now - feederRefillStartTime;
            
            // 🔥 CONDICIÓN 1: Peso suficiente alcanzado
            if (currentWeight >= targetWeight) {
                feederMotor->stopContinuous();
                feederMotor->disable();
                feederRefillInProgress = false;
                Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"status\":\"TARGET_REACHED\",\"final_weight\":" + 
                               String(currentWeight) + ",\"weight_added\":" + String(weightIncrease) + 
                               ",\"duration_ms\":" + String(elapsedTime) + "}");
            }
            // 🔥 CONDICIÓN 2: Aumento mínimo detectado (+5g)
            else if (weightIncrease >= 5.0) {
                feederMotor->stopContinuous();
                feederMotor->disable();
                feederRefillInProgress = false;
                Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"status\":\"SUFFICIENT_INCREASE\",\"final_weight\":" + 
                               String(currentWeight) + ",\"weight_added\":" + String(weightIncrease) + 
                               ",\"duration_ms\":" + String(elapsedTime) + "}");
            }
            // 🔥 CONDICIÓN 3: Timeout alcanzado (5 segundos)
            else if (elapsedTime >= FEEDER_REFILL_MAX_MS) {
                feederMotor->stopContinuous();
                feederMotor->disable();
                feederRefillInProgress = false;
                Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"status\":\"TIMEOUT\",\"final_weight\":" + 
                               String(currentWeight) + ",\"weight_added\":" + String(weightIncrease) + 
                               ",\"duration_ms\":" + String(elapsedTime) + "}");
            }
            // 🔥 CONDICIÓN 4: Emergencia - gato se acerca
            else if (!isFeederSafeToOperate()) {
                feederMotor->stopContinuous();
                feederMotor->disable();
                feederRefillInProgress = false;
                Serial.println("{\"auto_action\":\"FEEDER_REFILL\",\"status\":\"EMERGENCY_STOP\",\"reason\":\"CAT_DETECTED\",\"final_weight\":" + 
                               String(currentWeight) + ",\"weight_added\":" + String(weightIncrease) + 
                               ",\"duration_ms\":" + String(elapsedTime) + "}");
            }
            
            // 🔥 DEBUG: Progreso cada 1 segundo durante el refill
            static unsigned long lastProgressLog = 0;
            if (now - lastProgressLog >= 1000) {
                Serial.println("{\"refill_progress\":{\"elapsed_ms\":" + String(elapsedTime) + 
                               ",\"weight\":" + String(currentWeight) + ",\"weight_added\":" + 
                               String(weightIncrease) + ",\"target\":" + String(targetWeight) + "}}");
                lastProgressLog = now;
            }
        }
        
        // ===== AUTO-RELLENADO DEL COMEDERO (CADA 3 MINUTOS) - Solo si no está en progreso =====
        if (feederEnabled && !manualFeederControl && !feederRefillInProgress) {
            float weight = sensorManager->getFeederWeight();
            
            // Si peso < objetivo y pasaron 3 minutos desde el último intento
            if (weight < targetWeight && (now - lastFeederRetry >= 180000)) { // 180000ms = 3 minutos
                attemptFeederRefill();
                lastFeederRetry = now; // 🔥 Actualizar timestamp
            }
        }
        
        // ===== AUTO-CONTROL DEL BEBEDERO (COMPLETAMENTE AUTOMÁTICO) =====
        String waterLevel = sensorManager->getWaterLevel();
        bool catNearWater = sensorManager->isCatDrinking();
        
        // 🔥 ACTIVAR BOMBA: Si agua NO está al máximo Y no hay gato
        if (waterLevel != "FLOOD" && !catNearWater && !waterPump->isPumpRunning()) {
            waterPump->turnOn(30000); // 30 segundos máximo
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