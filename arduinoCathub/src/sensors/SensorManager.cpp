#include "SensorManager.h"

SensorManager::SensorManager() : 
    feederWeight_(nullptr), feederUltrasonic1_(nullptr), feederUltrasonic2_(nullptr), feederMotor_(nullptr),
    waterDispenser_(nullptr), waterDispenserIR_(nullptr), waterDispenserPump_(nullptr), 
    litterboxDHT_(nullptr), litterboxUltrasonic_(nullptr), litterboxMQ2_(nullptr), litterboxMotor_(nullptr) {}

void SensorManager::begin() {
    Serial.println(F("{\"event\":\"INITIALIZING_SENSORS_BY_DEVICE\"}"));
    
    // === COMEDERO ===
    Serial.println(F("{\"device\":\"FEEDER\",\"status\":\"INITIALIZING\"}"));
    
    // Peso del comedero
    feederWeight_ = new FeederWeightSensor();
    if (!feederWeight_->initialize()) {
        Serial.println(F("{\"device\":\"FEEDER\",\"sensor\":\"WEIGHT\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"FEEDER\",\"sensor\":\"WEIGHT\",\"status\":\"READY\"}"));
    }
    
    // Ultrasónicos del comedero
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
    
    // Motor del comedero
    feederMotor_ = new FeederStepperMotor();
    if (!feederMotor_->initialize()) {
        Serial.println(F("{\"device\":\"FEEDER\",\"motor\":\"STEPPER\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"FEEDER\",\"motor\":\"STEPPER\",\"status\":\"READY\"}"));
    }
    
    // === BEBEDERO ===
    Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"status\":\"INITIALIZING\"}"));
    
    waterDispenser_ = new WaterDispenserSensor();
    if (!waterDispenser_->initialize()) {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"sensor\":\"WATER_LEVEL\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"sensor\":\"WATER_LEVEL\",\"status\":\"READY\"}"));
    }
    
    waterDispenserIR_ = new WaterDispenserIRSensor();
    if (!waterDispenserIR_->initialize()) {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"sensor\":\"IR\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"sensor\":\"IR\",\"status\":\"READY\"}"));
    }
    
    waterDispenserPump_ = new WaterDispenserPump();
    if (!waterDispenserPump_->initialize()) {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"pump\":\"MOSFET\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"WATER_DISPENSER\",\"pump\":\"MOSFET\",\"status\":\"READY\"}"));
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
    
    litterboxMotor_ = new LitterboxStepperMotor();
    if (!litterboxMotor_->initialize()) {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"motor\":\"STEPPER\",\"status\":\"ERROR\"}"));
    } else {
        Serial.println(F("{\"device\":\"LITTERBOX\",\"motor\":\"STEPPER\",\"status\":\"READY\"}"));
    }
}

void SensorManager::poll() {
    // Comedero
    if (feederWeight_) feederWeight_->update();
    if (feederUltrasonic1_) feederUltrasonic1_->update();
    if (feederUltrasonic2_) feederUltrasonic2_->update();
    // Los motores no necesitan update constante
    
    // Bebedero
    if (waterDispenser_) waterDispenser_->update();
    if (waterDispenserIR_) waterDispenserIR_->update();
    if (waterDispenserPump_) waterDispenserPump_->update(); // Para auto-apagar
    
    // Arenero
    if (litterboxDHT_) litterboxDHT_->update();
    if (litterboxUltrasonic_) litterboxUltrasonic_->update();
    if (litterboxMQ2_) litterboxMQ2_->update();
    // Motor no necesita update constante
}

// Getters - Comedero
FeederWeightSensor* SensorManager::getFeederWeight() { return feederWeight_; }
FeederUltrasonicSensor1* SensorManager::getFeederUltrasonic1() { return feederUltrasonic1_; }
FeederUltrasonicSensor2* SensorManager::getFeederUltrasonic2() { return feederUltrasonic2_; }
FeederStepperMotor* SensorManager::getFeederMotor() { return feederMotor_; }

// Getters - Bebedero
WaterDispenserSensor* SensorManager::getWaterDispenser() { return waterDispenser_; }
WaterDispenserIRSensor* SensorManager::getWaterDispenserIR() { return waterDispenserIR_; }
WaterDispenserPump* SensorManager::getWaterDispenserPump() { return waterDispenserPump_; }

// Getters - Arenero
LitterboxDHTSensor* SensorManager::getLitterboxDHT() { return litterboxDHT_; }
LitterboxUltrasonicSensor* SensorManager::getLitterboxUltrasonic() { return litterboxUltrasonic_; }
LitterboxMQ2Sensor* SensorManager::getLitterboxMQ2() { return litterboxMQ2_; }
LitterboxStepperMotor* SensorManager::getLitterboxMotor() { return litterboxMotor_; }