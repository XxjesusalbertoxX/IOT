#include "CommandProcessor.h"

CommandProcessor::CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* motor) 
    : sensorManager(sensors), litterboxMotor(motor), initialized(false), 
      thresholdsConfiguredByRaspberry(false), lastConfigUpdate(0) {
    // 🔒 INICIALIZAR CON VALORES POR DEFECTO SEGUROS
    initializeDefaultThresholds();
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

    // Enrutar comandos
    if (device == "LITTERBOX") {
        processLitterboxCommand(cmd, params);
    } else if (device == "SENSOR") {
        processSensorCommand(cmd, params);
    } else if (device == "CONFIG") {
        processConfigCommand(cmd, params);
    } else if (device == "STATUS") {
        processStatusCommand();
    } else {
        Serial.println("{\"error\":\"UNKNOWN_DEVICE\",\"device\":\"" + device + "\"}");
    }
}

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
            
            // 🚨 AUTO-BLOQUEAR el motor hasta que se resuelvan los problemas
            if (!litterboxMotor->isBlocked()) {
                litterboxMotor->setState(-1); // BLOCKED
                Serial.println("{\"db_alert\":{\"type\":\"AUTO_BLOCKED\",\"device\":\"LITTERBOX\",\"reason\":\"UNSAFE_TO_CLEAN\",\"errors\":" + errors + ",\"timestamp\":" + String(millis()) + "}}");
            }
            return;
        }
        
        bool success = litterboxMotor->executeNormalCleaning();
        Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"NORMAL_CLEAN\",\"success\":" + String(success) + "}");
        
    } else if (command == "COMPLETE_CLEAN") {
        // ✅ VALIDACIÓN CRÍTICA: No limpiar si hay gato o está sucio
        if (!isLitterboxSafeToClean()) {
            String errors = getValidationErrors();
            Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"COMPLETE_CLEAN\",\"success\":false,\"error\":\"NOT_SAFE_TO_CLEAN\",\"details\":" + getSafetyStatus() + ",\"validation_errors\":" + errors + "}");
            
            // 🚨 REPORTAR ERROR ESPECÍFICO PARA LA DB
            Serial.println("{\"db_alert\":{\"type\":\"CLEANING_BLOCKED\",\"device\":\"LITTERBOX\",\"action\":\"COMPLETE_CLEAN\",\"errors\":" + errors + ",\"timestamp\":" + String(millis()) + "}}");
            
            // 🚨 AUTO-BLOQUEAR el motor hasta que se resuelvan los problemas
            if (!litterboxMotor->isBlocked()) {
                litterboxMotor->setState(-1); // BLOCKED
                Serial.println("{\"db_alert\":{\"type\":\"AUTO_BLOCKED\",\"device\":\"LITTERBOX\",\"reason\":\"UNSAFE_TO_CLEAN\",\"errors\":" + errors + ",\"timestamp\":" + String(millis()) + "}}");
            }
            return;
        }
        
        bool success = litterboxMotor->executeCompleteCleaning();
        Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"COMPLETE_CLEAN\",\"success\":" + String(success) + "}");
        
    } else if (command == "STATUS") {
        Serial.println("{\"device\":\"LITTERBOX\"," + litterboxMotor->getStatus() + ",\"safety\":" + getSafetyStatus() + ",\"validation_errors\":" + getValidationErrors() + "}");
        
    } else if (command == "EMERGENCY_STOP") {
        litterboxMotor->emergencyStop();
        Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"EMERGENCY_STOP\",\"success\":true}");
        
    } else if (command == "FORCE_SET_STATE") {
        // ⚠️ COMANDO DE EMERGENCIA: Cambiar estado sin validaciones (solo para emergencias)
        int state = params.toInt();
        bool success = litterboxMotor->setState(state);
        Serial.println("{\"device\":\"LITTERBOX\",\"command\":\"FORCE_SET_STATE\",\"success\":" + String(success) + ",\"state\":" + String(state) + ",\"warning\":\"SAFETY_BYPASSED\"}");
        
    } else {
        Serial.println("{\"device\":\"LITTERBOX\",\"error\":\"UNKNOWN_COMMAND\",\"command\":\"" + command + "\"}");
    }
}

void CommandProcessor::processSensorCommand(String command, String params) {
    if (command == "READ_ALL") {
        sendAllSensorReadings();
    } else if (command == "SAFETY_CHECK") {
        Serial.println("{\"device\":\"SENSORS\",\"safety_check\":" + getSafetyStatus() + ",\"validation_errors\":" + getValidationErrors() + "}");
    } else {
        Serial.println("{\"device\":\"SENSORS\",\"error\":\"UNKNOWN_COMMAND\",\"command\":\"" + command + "\"}");
    }
}

void CommandProcessor::processConfigCommand(String command, String params) {
    if (command == "SET_THRESHOLDS") {
        int commaCount = 0;
        for (int i = 0; i < params.length(); i++) {
            if (params.charAt(i) == ',') commaCount++;
        }
        
        if (commaCount == 5) {
            int startIndex = 0;
            float values[6];
            
            for (int i = 0; i < 6; i++) {
                int commaIndex = params.indexOf(',', startIndex);
                if (commaIndex == -1 && i == 5) {
                    values[i] = params.substring(startIndex).toFloat();
                } else if (commaIndex != -1) {
                    values[i] = params.substring(startIndex, commaIndex).toFloat();
                    startIndex = commaIndex + 1;
                } else {
                    Serial.println("{\"device\":\"CONFIG\",\"error\":\"INVALID_THRESHOLD_FORMAT\",\"expected\":\"cat,gas,hum_max,hum_min,temp_max,temp_min\"}");
                    return;
                }
            }
            
            setThresholds(values[0], values[1], values[2], values[3], values[4], values[5]);
            thresholdsConfiguredByRaspberry = true;
            lastConfigUpdate = millis();
            
            Serial.println("{\"device\":\"CONFIG\",\"command\":\"SET_THRESHOLDS\",\"success\":true,\"source\":\"RASPBERRY_PI\",\"config\":" + getCurrentThresholds() + "}");
        } else {
            Serial.println("{\"device\":\"CONFIG\",\"error\":\"INVALID_THRESHOLD_FORMAT\",\"expected\":\"cat,gas,hum_max,hum_min,temp_max,temp_min\",\"received_commas\":" + String(commaCount) + "}");
        }
        
    } else if (command == "RESET_THRESHOLDS") {
        resetToDefaultThresholds();
        Serial.println("{\"device\":\"CONFIG\",\"command\":\"RESET_THRESHOLDS\",\"success\":true,\"source\":\"DEFAULT_VALUES\",\"config\":" + getCurrentThresholds() + "}");
        
    } else if (command == "GET_THRESHOLDS") {
        Serial.println("{\"device\":\"CONFIG\",\"command\":\"GET_THRESHOLDS\",\"config\":" + getCurrentThresholds() + ",\"source\":\"" + (thresholdsConfiguredByRaspberry ? "RASPBERRY_PI" : "DEFAULT_VALUES") + "\"}");
        
    } else {
        Serial.println("{\"device\":\"CONFIG\",\"error\":\"UNKNOWN_COMMAND\",\"command\":\"" + command + "\"}");
    }
}

void CommandProcessor::processStatusCommand() {
    Serial.println("{\"system_status\":{\"litterbox\":" + litterboxMotor->getStatus() + ",\"safety\":" + getSafetyStatus() + ",\"validation_errors\":" + getValidationErrors() + "}}");
}

// ===== VALIDACIONES DE SEGURIDAD =====
bool CommandProcessor::isLitterboxSafeToOperate() {
    // Verificar que los sensores estén listos
    if (!sensorManager->isLitterboxUltrasonicReady()) return false;
    if (!sensorManager->isLitterboxDHTReady()) return false;
    if (!sensorManager->isLitterboxMQ2Ready()) return false;
    
    return true;
}

bool CommandProcessor::isLitterboxSafeToClean() {
    // 1. Verificar que esté en estado READY
    if (!litterboxMotor->isReady()) return false;
    
    // 2. ✅ CRÍTICO: NO hay gato presente
    if (isCatPresent()) return false;
    
    // 3. ✅ CRÍTICO: Arenero está limpio (no sucio)
    if (!isLitterboxClean()) return false;
    
    return true;
}

// ===== VALIDACIONES ESPECÍFICAS CON DATOS REALES =====
bool CommandProcessor::isCatPresent() {
    float distance = sensorManager->getLitterboxDistance();
    
    if (distance < 0) {
        Serial.println("{\"warning\":\"ULTRASONIC_SENSOR_ERROR\",\"distance\":" + String(distance) + "}");
        return true; //si no podemos leer, asumimos que hay gato (más seguro)
    }
    
    // ✅ VALIDACIÓN REAL: Distancia normal 14cm, gato presente ≤ 6cm
    bool catDetected = (distance <= catPresentThreshold);
    
    if (catDetected) {
        Serial.println("{\"cat_detection\":{\"detected\":true,\"distance\":" + String(distance) + 
                      ",\"threshold\":" + String(catPresentThreshold) + ",\"status\":\"CAT_PRESENT\"}}");
    }
    
    return catDetected;
}

bool CommandProcessor::isLitterboxClean() {
    float gasPPM = sensorManager->getLitterboxGasPPM();
    float humidity = sensorManager->getLitterboxHumidity();
    float temperature = sensorManager->getLitterboxTemperature();
    
    // Validar que los sensores estén funcionando
    if (gasPPM < 0 || humidity < 0 || temperature < -900) {
        Serial.println("{\"warning\":\"SENSOR_READ_ERROR\",\"gas\":" + String(gasPPM) + 
                      ",\"humidity\":" + String(humidity) + ",\"temp\":" + String(temperature) + "}");
        return false; // SEGURIDAD: Si no podemos leer, asumimos que está sucio (más seguro)
    }
    
    // ✅ VALIDACIÓN REAL: Verificar niveles dentro de rangos "limpios"
    bool gasOK = (gasPPM <= maxGasPPM);
    bool humidityOK = (humidity <= maxHumidity && humidity >= minHumidity);
    bool temperatureOK = (temperature <= maxTemperature && temperature >= minTemperature);
    
    bool isClean = (gasOK && humidityOK && temperatureOK);
    
    // Log detallado de las condiciones
    Serial.println("{\"cleanliness_check\":{\"clean\":" + String(isClean) + 
                  ",\"gas\":{\"ppm\":" + String(gasPPM) + ",\"max\":" + String(maxGasPPM) + ",\"ok\":" + String(gasOK) + "}," +
                  "\"humidity\":{\"value\":" + String(humidity) + ",\"range\":\"" + String(minHumidity) + "-" + String(maxHumidity) + "\",\"ok\":" + String(humidityOK) + "}," +
                  "\"temperature\":{\"value\":" + String(temperature) + ",\"range\":\"" + String(minTemperature) + "-" + String(maxTemperature) + "\",\"ok\":" + String(temperatureOK) + "}}}");
    
    if (!isClean) {
        if (!gasOK) {
            Serial.println("{\"litterbox_alert\":{\"type\":\"HIGH_GAS\",\"ppm\":" + String(gasPPM) + ",\"max_allowed\":" + String(maxGasPPM) + "}}");
        }
        if (!humidityOK) {
            Serial.println("{\"litterbox_alert\":{\"type\":\"HUMIDITY_OUT_OF_RANGE\",\"humidity\":" + String(humidity) + ",\"range\":\"" + String(minHumidity) + "-" + String(maxHumidity) + "\"}}");
        }
        if (!temperatureOK) {
            Serial.println("{\"litterbox_alert\":{\"type\":\"TEMPERATURE_OUT_OF_RANGE\",\"temperature\":" + String(temperature) + ",\"range\":\"" + String(minTemperature) + "-" + String(maxTemperature) + "\"}}");
        }
    }
    
    return isClean;
}

String CommandProcessor::getValidationErrors() {
    String errors = "[";
    bool hasErrors = false;
    
    // ✅ VERIFICAR GATO PRESENTE CON DATOS REALES
    float distance = sensorManager->getLitterboxDistance();
    if (distance >= 0) { // Solo si el sensor funciona
        if (distance <= catPresentThreshold) {
            if (hasErrors) errors += ",";
            errors += "{\"type\":\"CAT_PRESENT\",\"message\":\"Gato detectado en el arenero\",\"distance\":" + String(distance, 2) + ",\"threshold\":" + String(catPresentThreshold) + ",\"sensor\":\"ULTRASONIC\"}";
            hasErrors = true;
        }
    } else {
        // Error del sensor ultrasónico
        if (hasErrors) errors += ",";
        errors += "{\"type\":\"SENSOR_ERROR\",\"message\":\"Sensor ultrasónico no responde\",\"sensor\":\"ULTRASONIC\"}";
        hasErrors = true;
    }
    
    // ✅ VERIFICAR NIVELES DE SUCIEDAD CON DATOS REALES
    float gasPPM = sensorManager->getLitterboxGasPPM();
    float humidity = sensorManager->getLitterboxHumidity();
    float temperature = sensorManager->getLitterboxTemperature();
    
    // Error de gas (arenero sucio)
    if (gasPPM >= 0) { // Solo si el sensor funciona
        if (gasPPM > maxGasPPM) {
            if (hasErrors) errors += ",";
            errors += "{\"type\":\"DIRTY_LITTER\",\"message\":\"Nivel de gas muy alto - arenero sucio\",\"gas_ppm\":" + String(gasPPM, 2) + ",\"max_allowed\":" + String(maxGasPPM) + ",\"sensor\":\"MQ2\"}";
            hasErrors = true;
        }
    } else {
        // Error del sensor MQ2
        if (hasErrors) errors += ",";
        errors += "{\"type\":\"SENSOR_ERROR\",\"message\":\"Sensor de gas no responde\",\"sensor\":\"MQ2\"}";
        hasErrors = true;
    }
    
    // Error de humedad
    if (humidity >= 0) { // Solo si el sensor funciona
        if (humidity > maxHumidity || humidity < minHumidity) {
            if (hasErrors) errors += ",";
            errors += "{\"type\":\"HUMIDITY_ISSUE\",\"message\":\"Humedad fuera de rango\",\"humidity\":" + String(humidity, 1) + ",\"range\":\"" + String(minHumidity) + "-" + String(maxHumidity) + "\",\"sensor\":\"DHT\"}";
            hasErrors = true;
        }
    } else {
        // Error del sensor DHT (humedad)
        if (hasErrors) errors += ",";
        errors += "{\"type\":\"SENSOR_ERROR\",\"message\":\"Sensor de humedad no responde\",\"sensor\":\"DHT_HUMIDITY\"}";
        hasErrors = true;
    }
    
    // Error de temperatura
    if (temperature > -900) { // Solo si el sensor funciona
        if (temperature > maxTemperature || temperature < minTemperature) {
            if (hasErrors) errors += ",";
            errors += "{\"type\":\"TEMPERATURE_ISSUE\",\"message\":\"Temperatura fuera de rango\",\"temperature\":" + String(temperature, 1) + ",\"range\":\"" + String(minTemperature) + "-" + String(maxTemperature) + "\",\"sensor\":\"DHT\"}";
            hasErrors = true;
        }
    } else {
        // Error del sensor DHT (temperatura)
        if (hasErrors) errors += ",";
        errors += "{\"type\":\"SENSOR_ERROR\",\"message\":\"Sensor de temperatura no responde\",\"sensor\":\"DHT_TEMPERATURE\"}";
        hasErrors = true;
    }
    
    errors += "]";
    return errors;
}

String CommandProcessor::getSafetyStatus() {
    float distance = sensorManager->getLitterboxDistance();
    float gasPPM = sensorManager->getLitterboxGasPPM();
    float humidity = sensorManager->getLitterboxHumidity();
    float temperature = sensorManager->getLitterboxTemperature();
    
    bool catPresent = isCatPresent();
    bool litterboxClean = isLitterboxClean();
    bool safeToClean = (!catPresent && litterboxClean);
    
    return "{\"cat_present\":" + String(catPresent) + 
           ",\"distance\":" + String(distance) + 
           ",\"litterbox_clean\":" + String(litterboxClean) + 
           ",\"gas_ppm\":" + String(gasPPM) + 
           ",\"humidity\":" + String(humidity) + 
           ",\"temperature\":" + String(temperature) + 
           ",\"safe_to_clean\":" + String(safeToClean) + 
           ",\"thresholds\":{\"cat\":" + String(catPresentThreshold) + 
           ",\"gas_max\":" + String(maxGasPPM) + 
           ",\"hum_range\":\"" + String(minHumidity) + "-" + String(maxHumidity) + 
           "\",\"temp_range\":\"" + String(minTemperature) + "-" + String(maxTemperature) + "\"}}";
}

void CommandProcessor::sendAllSensorReadings() {
    sensorManager->printAllSensorReadings();
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
    float weight = sensorManager->getFeederWeight();
    float catDistance = sensorManager->getFeederCatDistance();
    float foodDistance = sensorManager->getFeederFoodDistance();
    
    return "{\"weight\":" + String(weight) + 
           ",\"cat_eating\":" + String(isCatEating()) + 
           ",\"has_food\":" + String(hasFood()) + 
           ",\"needs_refill\":" + String(isFeederNeedsRefill()) + 
           ",\"motor_running\":" + String(feederMotorRunning) + 
           ",\"min_weight\":" + String(MIN_WEIGHT_GRAMS) + 
           ",\"max_weight\":" + String(maxWeightGrams) + "}";
}
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

void CommandProcessor::update() {
    // ...existing code... (mantener toda la lógica del arenero)
    
    // ✅ AÑADIR CONTROL DEL COMEDERO
    updateFeederControl();
}

void CommandProcessor::updateWaterDispenserControl() {
    unsigned long now = millis();

    // Verificar cada 5 segundos
    if (now - lastWaterCheck >= WATER_CHECK_INTERVAL) {
        String nivel = sensorManager->getWaterLevel(); // "DRY", "WET", "FLOOD"
        WaterDispenserPump* pump = sensorManager->getWaterPump();
        bool catDrinking = false;

        // Verifica si tienes el método en SensorManager:
        // bool isCatDrinking();
        if (sensorManager->isWaterIRReady()) {
            catDrinking = sensorManager->isCatDrinking();
        }

        // Si está seco, la bomba NO está encendida y NO hay gato, la enciende
        if (nivel == "DRY" && pump && !pump->isPumpRunning() && !catDrinking) {
            pump->turnOn(WATER_PUMP_TIMEOUT); // Enciende la bomba con timeout de seguridad
            waterPumpStartTime = now;
            waterPumpRunning = true;
            Serial.println("{\"water_auto\":{\"trigger\":\"LOW_WATER\",\"level\":\"DRY\",\"cat_present\":false}}");
        }

        // Si hay agua y la bomba está encendida, la apaga
        if ((nivel == "WET" || nivel == "FLOOD") && pump && pump->isPumpRunning()) {
            pump->turnOff();
            waterPumpRunning = false;
            Serial.println("{\"water_auto\":{\"complete\":true,\"final_level\":\"" + nivel + "\"}}");
        }

        // Si hay gato, la bomba NO debe encenderse (aunque esté seco)
        if (nivel == "DRY" && catDrinking && pump && pump->isPumpRunning()) {
            // Opcional: puedes pausar la bomba si el gato se acerca mientras está encendida
            pump->turnOff();
            waterPumpRunning = false;
            Serial.println("{\"water_auto\":{\"paused\":\"CAT_PRESENT\",\"reason\":\"Cat detected, pump stopped for safety\"}}");
        }

        lastWaterCheck = now;
    }

    // Si la bomba lleva mucho tiempo encendida y sigue seco, apaga y alerta
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