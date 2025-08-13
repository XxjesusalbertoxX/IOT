#include "SensorManager.h"

SensorManager::SensorManager() : 
    feederWeight_(nullptr), feederUltrasonic1_(nullptr), feederUltrasonic2_(nullptr),
    waterDispenser_(nullptr), litterboxDHT_(nullptr), litterboxUltrasonic_(nullptr),
    litterboxMQ2_(nullptr) {}

void SensorManager::begin() {
    Serial.println(F("{\"event\":\"INITIALIZING_SENSORS_BY_DEVICE\"}"));
    
    // === COMEDERO ===
    Serial.println(F("{\"device\":\"FEEDER\",\"status\":\"INITIALIZING\"}"));
    
    feederWeight_ = new FeederWeightSensor();
    if (!feederWeight_->initialize()) {
        Serial.println(F("{\"device\":\"FEEDER\",\"sensor\":\"WEIGHT\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"FEEDER\",\"sensor\":\"WEIGHT\",\"status\":\"READY\"}"));
    }
    
    feederUltrasonic1_ = new FeederUltrasonicSensor(4, 5, "FEEDER_LEVEL_1");
    feederUltrasonic2_ = new FeederUltrasonicSensor(6, 7, "FEEDER_LEVEL_2");
    
    if (!feederUltrasonic1_->initialize()) {
        Serial.println(F("{\"device\":\"FEEDER\",\"sensor\":\"ULTRASONIC_1\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"FEEDER\",\"sensor\":\"ULTRASONIC_1\",\"status\":\"READY\"}"));
    }
    
    if (!feederUltrasonic2_->initialize()) {
        Serial.println(F("{\"device\":\"FEEDER\",\"sensor\":\"ULTRASONIC_2\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"FEEDER\",\"sensor\":\"ULTRASONIC_2\",\"status\":\"READY\"}"));
    }
    
    // === BEBEDERO ===
    Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"status\":\"INITIALIZING\"}"));
    
    waterDispenser_ = new WaterDispenserSensor();
    if (!waterDispenser_->initialize()) {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"sensor\":\"WATER_LEVEL\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"sensor\":\"WATER_LEVEL\",\"status\":\"READY\"}"));
    }
    
    // === ARENERO ===
    Serial.println(F("{\"device\":\"LITTERBOX\",\"status\":\"INITIALIZING\"}"));
    
    litterboxDHT_ = new LitterboxDHTSensor();
    if (!litterboxDHT_->initialize()) {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"DHT\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"DHT\",\"status\":\"READY\"}"));
    }
    
    litterboxUltrasonic_ = new LitterboxUltrasonicSensor();
    if (!litterboxUltrasonic_->initialize()) {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"ULTRASONIC\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"ULTRASONIC\",\"status\":\"READY\"}"));
    }
    
    litterboxMQ2_ = new LitterboxMQ2Sensor();
    if (!litterboxMQ2_->initialize()) {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"MQ2\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"MQ2\",\"status\":\"READY\"}"));
    }
}

void SensorManager::poll() {
    // Comedero
    if (feederWeight_) feederWeight_->update();
    if (feederUltrasonic1_) feederUltrasonic1_->update();
    if (feederUltrasonic2_) feederUltrasonic2_->update();
    
    // Bebedero
    if (waterDispenser_) waterDispenser_->update();
    
    // Arenero
    if (litterboxDHT_) litterboxDHT_->update();
    if (litterboxUltrasonic_) litterboxUltrasonic_->update();
    if (litterboxMQ2_) litterboxMQ2_->update();
}

// Getters
FeederWeightSensor* SensorManager::getFeederWeight() { return feederWeight_; }
FeederUltrasonicSensor* SensorManager::getFeederUltrasonic1() { return feederUltrasonic1_; }
FeederUltrasonicSensor* SensorManager::getFeederUltrasonic2() { return feederUltrasonic2_; }
WaterDispenserSensor* SensorManager::getWaterDispenser() { return waterDispenser_; }
LitterboxDHTSensor* SensorManager::getLitterboxDHT() { return litterboxDHT_; }
LitterboxUltrasonicSensor* SensorManager::getLitterboxUltrasonic() { return litterboxUltrasonic_; }
LitterboxMQ2Sensor* SensorManager::getLitterboxMQ2() { return litterboxMQ2_; }