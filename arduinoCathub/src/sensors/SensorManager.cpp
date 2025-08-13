#include "SensorManager.h"

SensorManager::SensorManager() : 
    feederWeight_(nullptr), feederUltrasonic1_(nullptr), feederUltrasonic2_(nullptr), feederMotor_(nullptr),
    waterDispenser_(nullptr), waterDispenserIR_(nullptr), waterDispenserPump_(nullptr), 
    litterboxDHT_(nullptr), litterboxUltrasonic_(nullptr), litterboxMQ2_(nullptr), litterboxMotor_(nullptr) {}

void SensorManager::begin() {
    Serial.println(F("{\"event\":\"INITIALIZING_SENSORS_BY_DEVICE\"}"));
    
    // Bomba del bebedero
    waterDispenserPump_ = new WaterDispenserPump();
    if (!waterDispenserPump_->initialize()) {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"component\":\"PUMP\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"component\":\"PUMP\",\"status\":\"READY\"}"));
    }

    // Sensor IR del bebedero
    waterDispenserIR_ = new WaterDispenserIRSensor();
    if (!waterDispenserIR_->initialize()) {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"sensor\":\"IR\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"sensor\":\"IR\",\"status\":\"READY\"}"));
    }

    // === COMEDERO ===
    feederWeight_ = new FeederWeightSensor();
    if (!feederWeight_->initialize()) {
        Serial.println(F("{\"device\":\"FEEDER\",\"sensor\":\"WEIGHT\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"FEEDER\",\"sensor\":\"WEIGHT\",\"status\":\"READY\"}"));
    }
    
    // Los pines ya están definidos DENTRO de cada sensor
    feederUltrasonic1_ = new FeederUltrasonicSensor1();
    feederUltrasonic2_ = new FeederUltrasonicSensor2();
    
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
    
    // === BEBEDERO === (pines ya definidos internamente)
    waterDispenser_ = new WaterDispenserSensor();
    if (!waterDispenser_->initialize()) {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"sensor\":\"WATER_LEVEL\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"sensor\":\"WATER_LEVEL\",\"status\":\"READY\"}"));
    }
    
    // === ARENERO === (pines ya definidos internamente)
    litterboxDHT_ = new LitterboxDHTSensor();
    litterboxUltrasonic_ = new LitterboxUltrasonicSensor();
    litterboxMQ2_ = new LitterboxMQ2Sensor();
    
    // Inicializar todos los sensores del arenero...
    if (!litterboxDHT_->initialize()) {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"DHT\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"DHT\",\"status\":\"READY\"}"));
    }
    
    if (!litterboxUltrasonic_->initialize()) {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"ULTRASONIC\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"ULTRASONIC\",\"status\":\"READY\"}"));
    }
    
    if (!litterboxMQ2_->initialize()) {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"MQ2\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"sensor\":\"MQ2\",\"status\":\"READY\"}"));
    }
}

void SensorManager::poll() {
    if (feederWeight_) feederWeight_->update();
    if (feederUltrasonic1_) feederUltrasonic1_->update();
    if (feederUltrasonic2_) feederUltrasonic2_->update();
    if (waterDispenser_) waterDispenser_->update();
    if (litterboxDHT_) litterboxDHT_->update();
    if (litterboxUltrasonic_) litterboxUltrasonic_->update();
    if (litterboxMQ2_) litterboxMQ2_->update();
    if (waterDispenserIR_) waterDispenserIR_->update();
    if (waterDispenserPump_) waterDispenserPump_->update();
}

// Getters
FeederWeightSensor* SensorManager::getFeederWeight() { return feederWeight_; }
FeederUltrasonicSensor1* SensorManager::getFeederUltrasonic1() { return feederUltrasonic1_; }
FeederUltrasonicSensor2* SensorManager::getFeederUltrasonic2() { return feederUltrasonic2_; }
WaterDispenserSensor* SensorManager::getWaterDispenser() { return waterDispenser_; }
LitterboxDHTSensor* SensorManager::getLitterboxDHT() { return litterboxDHT_; }
LitterboxUltrasonicSensor* SensorManager::getLitterboxUltrasonic() { return litterboxUltrasonic_; }
LitterboxMQ2Sensor* SensorManager::getLitterboxMQ2() { return litterboxMQ2_; }
WaterDispenserIRSensor* SensorManager::getWaterDispenserIR() { return waterDispenserIR_; }
WaterDispenserPump* SensorManager::getWaterDispenserPump() { return waterDispenserPump_; }