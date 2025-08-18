#include "CommandProcessor.h"

CommandProcessor::CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* motor) 
    : sensorManager(sensors), litterboxMotor(motor), initialized(false),
      deviceIdentifier(""), deviceType_str(""), deviceConfigured(false),
      thresholdsConfiguredByRaspberry(false), lastConfigUpdate(0),
      maxWeightGrams(500.0f), minWeightGrams(150.0f),
      catEatingThreshold(15.0f), foodEmptyDistance(8.0f),
      foodFullDistance(2.0f), feederThresholdsConfigured(false),
      feederAutoRefillEnabled(true), lastFeederCheck(0),
      feederMotorStartTime(0), feederMotorRunning(false),
      lastWeightBeforeRefill(0.0), lastWaterCheck(0),
      waterPumpStartTime(0), waterPumpRunning(false) {
    
    initializeDefaultThresholds();
    
    // Inicializar arrays de identificadores
    for (int i = 0; i < MAX_SENSORS; i++) {
        sensorIdentifiers[i] = "";
        sensorConfigured[i] = false;
    }
}

bool CommandProcessor::initialize() {
    if (sensorManager == nullptr || litterboxMotor == nullptr) {
        Serial.println("{\"error\":\"SENSOR_MANAGER_NOT_INITIALIZED\"}");
        return false;
    }
    
    initialized = true;
    return true;
}

void CommandProcessor::processCommand(String command) {
    command.trim();
    if (command.length() == 0) return;

    if (command == "PING") {
        Serial.println("PONG");
        return;
    }

    int firstColon = command.indexOf(':');
    int secondColon = command.indexOf(':', firstColon + 1);
    
    if (firstColon == -1) {
        Serial.println("{\"error\":\"INVALID_COMMAND_FORMAT\"}");
        return;
    }

    String device = command.substring(0, firstColon);
    String cmd = (secondColon == -1) ? command.substring(firstColon + 1) : command.substring(firstColon + 1, secondColon);
    String params = (secondColon == -1) ? "" : command.substring(secondColon + 1);

    if (device == "DEVICE") {
        processDeviceCommand(cmd, params);
    } else if (device == "SENSOR") {
        processSensorCommand(cmd, params);
    } else if (device == "SENSORS") {  // Soporte para "SENSORS:READ_ALL"
        processSensorCommand(cmd, params);
    } else if (device == "LITTERBOX") {
        processLitterboxCommand(cmd, params);
    } else if (device == "FEEDER") {
        processFeederCommand(cmd, params);
    } else if (device == "CONFIG") {
        processConfigCommand(cmd, params);
    } else if (device == "STATUS") {
        processStatusCommand();
    } else {
        Serial.println("{\"error\":\"UNKNOWN_DEVICE\",\"device\":\"" + device + "\"}");
    }
}

void CommandProcessor::processDeviceCommand(String command, String params) {
    if (command == "CONFIGURE") {
        // Formato: "DEVICE:CONFIGURE:ARENERO-001,litterbox"
        int commaIndex = params.indexOf(',');
        if (commaIndex == -1) {
            Serial.println("{\"error\":\"INVALID_CONFIGURE_FORMAT\"}");
            return;
        }
        
        String identifier = params.substring(0, commaIndex);
        String deviceType = params.substring(commaIndex + 1);
        
        this->deviceIdentifier = identifier;
        this->deviceType_str = deviceType;
        this->deviceConfigured = true;
        
        Serial.println("{\"device\":\"DEVICE\",\"command\":\"CONFIGURE\",\"success\":true,\"identifier\":\"" + identifier + "\",\"type\":\"" + deviceType + "\"}");
    } else {
        Serial.println("{\"device\":\"DEVICE\",\"error\":\"UNKNOWN_COMMAND\"}");
    }
}

void CommandProcessor::processSensorCommand(String command, String params) {
    if (command == "SET_ID") {
        // Formato: "SENSOR:SET_ID:0,TEMP_ARENERO-001"
        int commaIndex = params.indexOf(',');
        if (commaIndex == -1) {
            Serial.println("{\"error\":\"INVALID_SET_ID_FORMAT\"}");
            return;
        }
        
        int sensorIndex = params.substring(0, commaIndex).toInt();
        String sensorId = params.substring(commaIndex + 1);
        
        if (sensorIndex >= 0 && sensorIndex < MAX_SENSORS) {
            this->sensorIdentifiers[sensorIndex] = sensorId;
            this->sensorConfigured[sensorIndex] = true;
            
            Serial.println("{\"device\":\"SENSOR\",\"command\":\"SET_ID\",\"success\":true,\"index\":" + String(sensorIndex) + ",\"identifier\":\"" + sensorId + "\"}");
        } else {
            Serial.println("{\"error\":\"INVALID_SENSOR_INDEX\"}");
        }
    } else if (command == "READ_ALL") {
        sendAllSensorReadingsWithIdentifiers();
    } else {
        Serial.println("{\"device\":\"SENSORS\",\"error\":\"UNKNOWN_COMMAND\"}");
    }
}

void CommandProcessor::sendAllSensorReadingsWithIdentifiers() {
    if (!deviceConfigured) {
        Serial.println("{\"error\":\"DEVICE_NOT_CONFIGURED\"}");
        return;
    }
    
    String deviceType = this->deviceType_str;
    
    if (deviceType == "litterbox") {
        String response = "{";
        response += "\"distance\":" + String(sensorManager->getLitterboxDistance()) + ",";
        response += "\"temperature\":" + String(sensorManager->getLitterboxTemperature()) + ",";
        response += "\"humidity\":" + String(sensorManager->getLitterboxHumidity()) + ",";
        response += "\"gas_ppm\":" + String(sensorManager->getLitterboxGasPPM());
        response += "}";
        Serial.println(response);
    } else if (deviceType == "feeder") {
        String response = "{";
        response += "\"weight\":" + String(sensorManager->getFeederWeight()) + ",";
        response += "\"cat_distance\":" + String(sensorManager->getFeederCatDistance()) + ",";
        response += "\"food_distance\":" + String(sensorManager->getFeederFoodDistance());
        response += "}";
        Serial.println(response);
    } else if (deviceType == "waterdispenser") {
        String response = "{";
        response += "\"water_level\":\"" + sensorManager->getWaterLevel() + "\",";
        response += "\"cat_drinking\":" + String(sensorManager->isCatDrinking());
        response += "}";
        Serial.println(response);
    } else {
        Serial.println("{\"error\":\"UNKNOWN_DEVICE_TYPE\"}");
    }
}

bool CommandProcessor::areAllSensorsReady() {
    return sensorManager->areAllSensorsReady();
}

// ===== GESTIÓN DE UMBRALES =====
void CommandProcessor::initializeDefaultThresholds() {
    catDetectionDistance = DEFAULT_CAT_DETECTION_DISTANCE;
    catPresentThreshold = DEFAULT_CAT_PRESENT_THRESHOLD;
    maxGasPPM = DEFAULT_MAX_GAS_PPM;
    maxHumidity = DEFAULT_MAX_HUMIDITY;
    minHumidity = DEFAULT_MIN_HUMIDITY;
    maxTemperature = DEFAULT_MAX_TEMPERATURE;
    minTemperature = DEFAULT_MIN_TEMPERATURE;
    thresholdsConfiguredByRaspberry = false;
}

void CommandProcessor::resetToDefaultThresholds() {
    initializeDefaultThresholds();
}

String CommandProcessor::getCurrentThresholds() {
    return "{\"cat_threshold\":" + String(catPresentThreshold) + 
           ",\"gas_max\":" + String(maxGasPPM) + 
           ",\"humidity_max\":" + String(maxHumidity) + 
           ",\"humidity_min\":" + String(minHumidity) + 
           ",\"temperature_max\":" + String(maxTemperature) + 
           ",\"temperature_min\":" + String(minTemperature) + 
           ",\"configured_by_raspberry\":" + String(thresholdsConfiguredByRaspberry) + "}";
}

void CommandProcessor::setThresholds(float catThreshold, float gasMax, float humMax, float humMin, float tempMax, float tempMin) {
    if (catThreshold > 0 && catThreshold < 20 &&
        gasMax > 0 && gasMax < 5000 &&
        humMax > humMin && humMax <= 100 &&
        humMin >= 0 && humMin < humMax &&
        tempMax > tempMin && tempMax <= 60 &&
        tempMin >= -10 && tempMin < tempMax) {
        
        catPresentThreshold = catThreshold;
        maxGasPPM = gasMax;
        maxHumidity = humMax;
        minHumidity = humMin;
        maxTemperature = tempMax;
        minTemperature = tempMin;
        
        Serial.println("{\"threshold_update\":\"SUCCESS\",\"values\":" + getCurrentThresholds() + "}");
    } else {
        Serial.println("{\"threshold_update\":\"FAILED\",\"error\":\"INVALID_VALUES\",\"keeping\":\"CURRENT_VALUES\"}");
    }
}

bool CommandProcessor::areThresholdsConfigured() const {
    return thresholdsConfiguredByRaspberry;
}

void CommandProcessor::update() {
    // ✅ VERIFICACIÓN CONTINUA DE SEGURIDAD CADA 2 SEGUNDOS
    static unsigned long lastSafetyCheck = 0;
    unsigned long now = millis();
    
    if (now - lastSafetyCheck >= 2000) { // Cada 2 segundos
        
        // Solo verificar si el motor está activo (READY)
        if (litterboxMotor->isReady()) {
            
            // 🚨 AUTO-BLOQUEAR si detecta gato presente
            if (isCatPresent()) {
                float distance = sensorManager->getLitterboxDistance();
                litterboxMotor->setState(-1); // BLOCKED
                Serial.println("{\"db_alert\":{\"type\":\"AUTO_BLOCKED_CAT\",\"device\":\"LITTERBOX\",\"distance\":" + String(distance) + ",\"threshold\":" + String(catPresentThreshold) + ",\"timestamp\":" + String(millis()) + "}}");
            }
            
            // 🚨 AUTO-BLOQUEAR si detecta arenero muy sucio
            else if (!isLitterboxClean()) {
                float gasPPM = sensorManager->getLitterboxGasPPM();
                float humidity = sensorManager->getLitterboxHumidity();
                float temperature = sensorManager->getLitterboxTemperature();
                
                litterboxMotor->setState(-1); // BLOCKED
                Serial.println("{\"db_alert\":{\"type\":\"AUTO_BLOCKED_DIRTY\",\"device\":\"LITTERBOX\",\"gas_ppm\":" + String(gasPPM) + ",\"humidity\":" + String(humidity) + ",\"temperature\":" + String(temperature) + ",\"timestamp\":" + String(millis()) + "}}");
            }
        }
        
        // 🔄 AUTO-DESBLOQUEAR si las condiciones vuelven a ser seguras
        else if (litterboxMotor->isBlocked()) {
            // Verificar que todas las condiciones sean seguras
            if (!isCatPresent() && isLitterboxClean() && areAllSensorsReady()) {
                litterboxMotor->setState(0); // EMPTY (la Raspberry Pi debe cambiar a READY)
                Serial.println("{\"db_alert\":{\"type\":\"AUTO_UNBLOCKED\",\"device\":\"LITTERBOX\",\"message\":\"Condiciones seguras restablecidas - estado EMPTY\",\"distance\":" + String(sensorManager->getLitterboxDistance()) + ",\"gas_ppm\":" + String(sensorManager->getLitterboxGasPPM()) + ",\"timestamp\":" + String(millis()) + "}}");
            }
        }
        
        lastSafetyCheck = now;
    }
    
    // ✅ ENVÍO DE DATOS EN TIEMPO REAL cada 5 segundos para monitoreo
    static unsigned long lastDataSend = 0;
    if (now - lastDataSend >= 5000) {
        Serial.println("{\"real_time_data\":" + sensorManager->getAllReadings() + ",\"motor_state\":" + String(litterboxMotor->getState()) + "}");
        lastDataSend = now;
    }
    
    updateFeederControl();
}

// ...existing code... (mantener todo el código del arenero anterior)

void CommandProcessor::processFeederCommand(String command, String params) {
    if (command == "SET_MAX_WEIGHT") {
        float maxWeight = params.toFloat();
        if (maxWeight > MIN_WEIGHT_GRAMS && maxWeight <= 1000.0f) {
            maxWeightGrams = maxWeight;
            Serial.println("{\"device\":\"FEEDER\",\"command\":\"SET_MAX_WEIGHT\",\"success\":true,\"max_weight\":" + String(maxWeight) + "}");
        } else {
            Serial.println("{\"device\":\"FEEDER\",\"command\":\"SET_MAX_WEIGHT\",\"success\":false,\"error\":\"INVALID_WEIGHT\",\"must_be\":\">" + String(MIN_WEIGHT_GRAMS) + " and <=1000\"}");
        }
        
    } else if (command == "STATUS") {
        Serial.println("{\"device\":\"FEEDER\",\"status\":" + getFeederStatus() + "}");
        
    // 🔧 MANTENER PARA FUTURAS MEJORAS (pero no activos)
    } else if (command == "MANUAL_FEED") {
        Serial.println("{\"device\":\"FEEDER\",\"error\":\"NOT_IMPLEMENTED\",\"message\":\"Solo auto-rellenado disponible\"}");
    } else if (command == "MANUAL_REFILL") {
        Serial.println("{\"device\":\"FEEDER\",\"error\":\"NOT_IMPLEMENTED\",\"message\":\"Solo auto-rellenado disponible\"}");
        
    } else {
        Serial.println("{\"device\":\"FEEDER\",\"error\":\"UNKNOWN_COMMAND\",\"command\":\"" + command + "\"}");
    }
}

// ===== AUTO-RELLENADO INTELIGENTE =====
void CommandProcessor::updateFeederControl() {
    unsigned long now = millis();
    
    // ✅ VERIFICAR AUTO-RELLENADO cada 10 segundos
    if (now - lastFeederCheck >= 10000) {
        
        // 🔍 Si peso < 150g Y no está rellenando Y hay comida disponible
        if (isFeederNeedsRefill() && !feederMotorRunning && hasFood()) {
            Serial.println("{\"auto_refill\":{\"trigger\":\"LOW_WEIGHT\",\"current\":" + String(sensorManager->getFeederWeight()) + ",\"min\":" + String(MIN_WEIGHT_GRAMS) + "}}");
            startAutoRefill();
        }
        
        // 🚨 Si no hay comida en el compartimento
        else if (isFeederNeedsRefill() && !hasFood()) {
            Serial.println("{\"db_alert\":{\"type\":\"NO_FOOD_CONTAINER\",\"device\":\"FEEDER\",\"weight\":" + String(sensorManager->getFeederWeight()) + ",\"food_distance\":" + String(sensorManager->getFeederFoodDistance()) + ",\"timestamp\":" + String(millis()) + "}}");
        }
        
        lastFeederCheck = now;
    }
    
    // ✅ CONTROL DEL MOTOR AUTO-RELLENANDO
    if (feederMotorRunning) {
        float currentWeight = sensorManager->getFeederWeight();
        
        // 🎯 PARAR cuando llegue al máximo
        if (currentWeight >= maxWeightGrams) {
            stopAutoRefill();
            Serial.println("{\"auto_refill\":{\"complete\":true,\"final_weight\":" + String(currentWeight) + ",\"target\":" + String(maxWeightGrams) + "}}");
        }
        
        // ⏰ TIMEOUT de seguridad
        checkFeederTimeout();
    }
    
    // 🐱 DETECTAR GATO COMIENDO cada 5 segundos
    static unsigned long lastCatCheck = 0;
    if (now - lastCatCheck >= CAT_DETECTION_INTERVAL) {
        if (isCatEating()) {
            Serial.println("{\"db_alert\":{\"type\":\"CAT_EATING\",\"device\":\"FEEDER\",\"distance\":" + String(sensorManager->getFeederCatDistance()) + ",\"weight\":" + String(sensorManager->getFeederWeight()) + ",\"timestamp\":" + String(millis()) + "}}");
        }
        lastCatCheck = now;
    }
}

bool CommandProcessor::isFeederNeedsRefill() {
    float currentWeight = sensorManager->getFeederWeight();
    return (currentWeight >= 0 && currentWeight < MIN_WEIGHT_GRAMS); // < 150g
}

bool CommandProcessor::isCatEating() {
    float catDistance = sensorManager->getFeederCatDistance();
    return (catDistance >= 0 && catDistance <= 15.0f); // Gato cerca del comedero
}

bool CommandProcessor::hasFood() {
    float foodDistance = sensorManager->getFeederFoodDistance();
    return (foodDistance >= 0 && foodDistance < 8.0f); // Hay comida en compartimento
}

void CommandProcessor::startAutoRefill() {
    FeederStepperMotor* motor = sensorManager->getFeederMotor();
    if (motor && hasFood()) {
        motor->enable();
        feederMotorRunning = true;
        feederMotorStartTime = millis();
        lastWeightBeforeRefill = sensorManager->getFeederWeight();
        
        Serial.println("{\"auto_refill\":{\"start\":true,\"initial_weight\":" + String(lastWeightBeforeRefill) + ",\"target\":" + String(maxWeightGrams) + "}}");
    }
}

void CommandProcessor::stopAutoRefill() {
    FeederStepperMotor* motor = sensorManager->getFeederMotor();
    if (motor) motor->disable();
    
    feederMotorRunning = false;
    Serial.println("{\"auto_refill\":{\"stop\":true,\"final_weight\":" + String(sensorManager->getFeederWeight()) + "}}");
}

String CommandProcessor::getFeederStatus() {

    float currentWeight = sensorManager->getFeederWeight();
    float catDistance = sensorManager->getFeederCatDistance();
    float foodDistance = sensorManager->getFeederFoodDistance();
    
    bool needsRefill = isFeederNeedsRefill();
    bool catEating = isCatEating();
    bool hasFood = this->hasFood();
    bool safeToOperate = isFeederSafeToOperate();
    
    String status = "{";
    status += "\"weight_grams\":" + String(currentWeight) + ",";
    status += "\"cat_distance\":" + String(catDistance) + ",";
    status += "\"food_distance\":" + String(foodDistance) + ",";
    status += "\"needs_refill\":" + String(needsRefill) + ",";
    status += "\"cat_eating\":" + String(catEating) + ",";
    status += "\"has_food\":" + String(hasFood) + ",";
    status += "\"safe_to_operate\":" + String(safeToOperate) + ",";
    status += "\"auto_refill_enabled\":" + String(feederAutoRefillEnabled) + ",";
    status += "\"motor_running\":" + String(feederMotorRunning) + ",";
    status += "\"thresholds\":{";
    status += "\"min_weight\":" + String(minWeightGrams) + ",";
    status += "\"max_weight\":" + String(maxWeightGrams) + ",";
    status += "\"cat_eating_threshold\":" + String(catEatingThreshold) + ",";
    status += "\"food_empty_distance\":" + String(foodEmptyDistance);
    status += "}}";
    
    return status;
}

void CommandProcessor::startFeederRefill() {
    if (!isFeederSafeToOperate()) {
        Serial.println("{\"feeder_error\":\"NOT_SAFE_TO_START_REFILL\"}");
        return;
    }
    
    FeederStepperMotor* motor = sensorManager->getFeederMotor();
    if (motor) {
        motor->enable();
        feederMotorRunning = true;
        feederMotorStartTime = millis();
        lastWeightBeforeRefill = sensorManager->getFeederWeight();
        
        Serial.println("{\"feeder_action\":\"REFILL_START\",\"initial_weight\":" + String(lastWeightBeforeRefill) + ",\"target_weight\":" + String(maxWeightGrams) + "}");
    }
}

void CommandProcessor::stopFeederRefill() {
    FeederStepperMotor* motor = sensorManager->getFeederMotor();
    if (motor) {
        motor->disable();
    }
    
    feederMotorRunning = false;
    float finalWeight = sensorManager->getFeederWeight();
    
    Serial.println("{\"feeder_action\":\"REFILL_STOP\",\"final_weight\":" + String(finalWeight) + ",\"duration_ms\":" + String(millis() - feederMotorStartTime) + "}");
}

void CommandProcessor::checkFeederTimeout() {
    unsigned long now = millis();
    unsigned long runTime = now - feederMotorStartTime;
    
    // Verificar timeout
    if (runTime >= FEEDER_TIMEOUT_MS) {
        float currentWeight = sensorManager->getFeederWeight();
        float weightDifference = currentWeight - lastWeightBeforeRefill;
        
        stopFeederRefill();
        
        if (weightDifference < 10.0) { // Si no aumentó al menos 10 gramos
            Serial.println("{\"feeder_error\":{\"type\":\"FEEDER_JAMMED\",\"message\":\"Motor funcionó pero no aumentó peso - posible atasco\",\"runtime_ms\":" + String(runTime) + ",\"weight_diff\":" + String(weightDifference) + "}}");
            Serial.println("{\"db_alert\":{\"type\":\"FEEDER_JAMMED\",\"device\":\"FEEDER\",\"runtime_ms\":" + String(runTime) + ",\"weight_increase\":" + String(weightDifference) + ",\"timestamp\":" + String(millis()) + "}}");
        } else {
            Serial.println("{\"feeder_error\":{\"type\":\"FEEDER_TIMEOUT\",\"message\":\"Motor se detuvo por timeout\",\"runtime_ms\":" + String(runTime) + "}}");
        }
    }
    
    // Si el motor está corriendo, hacerlo girar
    if (feederMotorRunning) {
        FeederStepperMotor* motor = sensorManager->getFeederMotor();
        if (motor && motor->isEnabled()) {
            motor->feedPortion(1); // Dispensar una porción pequeña continuamente
            delay(500); // Pausa entre porciones
        }
    }
}

void CommandProcessor::setFeederThresholds(float minWeight, float maxWeight, float catThreshold, float emptyDistance, float fullDistance) {
    if (minWeight > 0 && maxWeight > minWeight && catThreshold > 0 && emptyDistance > fullDistance) {
        minWeightGrams = minWeight;
        maxWeightGrams = maxWeight;
        catEatingThreshold = catThreshold;
        foodEmptyDistance = emptyDistance;
        foodFullDistance = fullDistance;
        feederThresholdsConfigured = true;
        
        Serial.println("{\"feeder_threshold_update\":\"SUCCESS\",\"thresholds\":{\"min_weight\":" + String(minWeight) + ",\"max_weight\":" + String(maxWeight) + ",\"cat_threshold\":" + String(catThreshold) + ",\"empty_distance\":" + String(emptyDistance) + ",\"full_distance\":" + String(fullDistance) + "}}");
    } else {
        Serial.println("{\"feeder_threshold_update\":\"FAILED\",\"error\":\"INVALID_VALUES\"}");
    }
}

void CommandProcessor::updateWaterDispenserControl() {
    unsigned long now = millis();

    if (now - lastWaterCheck >= WATER_CHECK_INTERVAL) {
        String nivel = sensorManager->getWaterLevel();
        bool catDrinking = false;
        WaterDispenserPump* pump = sensorManager->getWaterPump(); // <--- AGREGA ESTA LÍNEA

        if (sensorManager->isWaterIRReady()) {
            catDrinking = sensorManager->isCatDrinking();
        }

        if (nivel == "DRY" && pump && !pump->isPumpRunning() && !catDrinking) {
            pump->turnOn(WATER_PUMP_TIMEOUT);
            waterPumpStartTime = now;
            waterPumpRunning = true;
            Serial.println("{\"water_auto\":{\"trigger\":\"LOW_WATER\",\"level\":\"DRY\",\"cat_present\":false}}");
        }

        if ((nivel == "WET" || nivel == "FLOOD") && pump && pump->isPumpRunning()) {
            pump->turnOff();
            waterPumpRunning = false;
            Serial.println("{\"water_auto\":{\"complete\":true,\"final_level\":\"" + nivel + "\"}}");
        }

        if (nivel == "DRY" && catDrinking && pump && pump->isPumpRunning()) {
            pump->turnOff();
            waterPumpRunning = false;
            Serial.println("{\"water_auto\":{\"paused\":\"CAT_PRESENT\",\"reason\":\"Cat detected, pump stopped for safety\"}}");
        }

        lastWaterCheck = now;
    }

    // Timeout de la bomba
    WaterDispenserPump* pump = sensorManager->getWaterPump();
    if (pump && pump->isPumpRunning() && (millis() - waterPumpStartTime > WATER_PUMP_TIMEOUT)) {
        String nivel = sensorManager->getWaterLevel();
        pump->turnOff();
        waterPumpRunning = false;
        if (nivel == "DRY") {
            Serial.println("{\"water_error\":\"NO_WATER_IN_RESERVOIR\"}");
        }
    }
}