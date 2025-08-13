#include "CommandProcessor.h"

CommandProcessor::CommandProcessor(ConfigStore& config, SensorManager& sensors) 
    : configStore(config), sensorManager(sensors) {}

void CommandProcessor::begin() {
    Serial.println(F("{\"event\":\"COMMAND_PROCESSOR_READY\"}"));
}

void CommandProcessor::poll() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            processCommand(command);
        }
    }
}

void CommandProcessor::processCommand(String command) {
    command.trim();
    command.toUpperCase();
    
    if (command == "GET_WEIGHT") {
        sendWeightData();
    }
    else if (command == "TARE") {
        tareWeightSensor();
    }
    else if (command.startsWith("CALIBRATE:")) {
        float weight = command.substring(10).toFloat();
        calibrateWeightSensor(weight);
    }
    else if (command == "WEIGHT_STATUS" || command == "STATUS") {
        WeightSensor* ws = sensorManager.getWeightSensor();
        if (ws) {
            Serial.println("{\"sensor\":\"weight\",\"ready\":" + String(ws->isReady() ? "true" : "false") + ",\"status\":\"" + ws->getStatus() + "\"}");
        }
    }
    else if (command == "HELP") {
        showHelp();
    }
    else {
        Serial.println("{\"error\":\"UNKNOWN_COMMAND\",\"command\":\"" + command + "\"}");
    }
}

void CommandProcessor::sendWeightData() {
    WeightSensor* ws = sensorManager.getWeightSensor();
    if (ws) {
        float weight = ws->getCurrentWeight();
        Serial.println("{\"sensor\":\"weight\",\"value\":" + String(weight, 2) + ",\"unit\":\"g\",\"status\":\"" + ws->getStatus() + "\"}");
    } else {
        Serial.println("{\"sensor\":\"weight\",\"error\":\"NOT_AVAILABLE\"}");
    }
}

void CommandProcessor::tareWeightSensor() {
    WeightSensor* ws = sensorManager.getWeightSensor();
    if (ws) {
        ws->tare();
        Serial.println("{\"action\":\"tare\",\"status\":\"OK\"}");
    } else {
        Serial.println("{\"action\":\"tare\",\"status\":\"ERROR\",\"message\":\"Sensor not available\"}");
    }
}

void CommandProcessor::calibrateWeightSensor(float knownWeight) {
    if (knownWeight > 0) {
        WeightSensor* ws = sensorManager.getWeightSensor();
        if (ws) {
            ws->calibrate(knownWeight);
            Serial.println("{\"action\":\"calibrate\",\"weight\":" + String(knownWeight) + ",\"status\":\"OK\"}");
        } else {
            Serial.println("{\"action\":\"calibrate\",\"status\":\"ERROR\",\"message\":\"Sensor not available\"}");
        }
    } else {
        Serial.println("{\"action\":\"calibrate\",\"status\":\"ERROR\",\"message\":\"Invalid weight\"}");
    }
}

void CommandProcessor::showHelp() {
    Serial.println("{\"help\":\"available_commands\",\"commands\":[");
    Serial.println("  \"GET_WEIGHT - Get weight sensor reading\",");
    Serial.println("  \"TARE - Reset weight scale to zero\",");
    Serial.println("  \"CALIBRATE:XXX - Calibrate weight sensor with XXX grams\",");
    Serial.println("  \"STATUS - Get sensor status\",");
    Serial.println("  \"HELP - Show this help\"");
    Serial.println("]}");
}