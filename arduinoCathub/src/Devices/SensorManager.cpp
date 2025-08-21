// SensorManager.cpp
#include "SensorManager.h"
#include "config/DeviceIDs.h"
#include "feeder/config/SensorIDs.h"
#include "feeder/config/ActuatorIDs.h"
#include "litterbox/config/SensorIDs.h"
#include "litterbox/config/ActuatorIDs.h"
#include "waterdispenser/config/SensorIDs.h"
#include "waterdispenser/config/ActuatorIDs.h"

SensorManager::SensorManager(LitterboxUltrasonicSensor* litterboxUltrasonic,
                             LitterboxDHTSensor* litterboxDHT,
                             LitterboxMQ2Sensor* litterboxMQ2,
                             LitterboxStepperMotor* litterboxMotorPtr,
                             FeederWeightSensor* feederWeight,
                             FeederUltrasonicSensor1* feederUltrasonic1Ptr,
                             FeederUltrasonicSensor2* feederUltrasonic2Ptr,
                             FeederStepperMotor* feederMotorPtr,
                             WaterDispenserSensor* waterSensorPtr,
                             WaterDispenserPump* waterPumpPtr,
                             WaterDispenserIRSensor* waterIRSensorPtr)
    : initialized(false),
      lastUpdateTime(0) {
    ultrasonicSensor = litterboxUltrasonic;
    dhtSensor = litterboxDHT;
    mq2Sensor = litterboxMQ2;
    litterboxMotor = litterboxMotorPtr;

    weightSensor = feederWeight;
    feederUltrasonic1 = feederUltrasonic1Ptr;
    feederUltrasonic2 = feederUltrasonic2Ptr;
    feederMotor = feederMotorPtr;

    waterSensor = waterSensorPtr;
    waterPump = waterPumpPtr;
    waterIRSensor = waterIRSensorPtr;
}

SensorManager::~SensorManager() {
    // No eliminamos objetos externos, solo limpiamos referencias por seguridad
    ultrasonicSensor = nullptr;
    dhtSensor = nullptr;
    mq2Sensor = nullptr;
    litterboxMotor = nullptr;
    weightSensor = nullptr;
    feederUltrasonic1 = nullptr;
    feederUltrasonic2 = nullptr;
    feederMotor = nullptr;
    waterSensor = nullptr;
    waterPump = nullptr;
    waterIRSensor = nullptr;
}

bool SensorManager::begin() {
    Serial.println("{\"sensor_manager\":\"INITIALIZING\"}");

    bool ultrasonicOK    = (ultrasonicSensor != nullptr) ? ultrasonicSensor->initialize() : false;
    bool dhtOK           = (dhtSensor != nullptr) ? dhtSensor->initialize() : false;
    bool mq2OK           = (mq2Sensor != nullptr) ? mq2Sensor->initialize() : false;
    bool litterboxMotorOK= (litterboxMotor != nullptr) ? litterboxMotor->initialize() : false;

    bool weightOK        = (weightSensor != nullptr) ? weightSensor->initialize() : false;
    bool feederUltr1OK   = (feederUltrasonic1 != nullptr) ? feederUltrasonic1->initialize() : false;
    bool feederUltr2OK   = (feederUltrasonic2 != nullptr) ? feederUltrasonic2->initialize() : false;
    bool feederMotorOK   = (feederMotor != nullptr) ? feederMotor->initialize() : false;

    bool waterSensorOK   = (waterSensor != nullptr) ? waterSensor->initialize() : false;
    bool waterPumpOK     = (waterPump != nullptr) ? waterPump->initialize() : false;
    bool waterIROK       = (waterIRSensor != nullptr) ? waterIRSensor->initialize() : false;

    Serial.println("{\"sensor\":\"LITTERBOX_ULTRASONIC\",\"status\":\"" + String(ultrasonicOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"DHT\",\"status\":\"" + String(dhtOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"MQ2\",\"status\":\"" + String(mq2OK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"LITTERBOX_MOTOR\",\"status\":\"" + String(litterboxMotorOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"FEEDER_WEIGHT\",\"status\":\"" + String(weightOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"FEEDER_ULTRASONIC_CAT\",\"status\":\"" + String(feederUltr1OK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"FEEDER_ULTRASONIC_FOOD\",\"status\":\"" + String(feederUltr2OK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"FEEDER_MOTOR\",\"status\":\"" + String(feederMotorOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"WATER_SENSOR\",\"status\":\"" + String(waterSensorOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"WATER_PUMP\",\"status\":\"" + String(waterPumpOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"WATER_IR\",\"status\":\"" + String(waterIROK ? "OK" : "FAILED") + "\"}");

    initialized = true;
    Serial.println("{\"sensor_manager\":\"READY\",\"all_systems\":\"INITIALIZED\"}");
    return initialized;
}

float SensorManager::getFeederWeight() {
    if (weightSensor && weightSensor->isReady()) return weightSensor->getCurrentWeight();
    return 0.0f;
}

WaterDispenserSensor* SensorManager::getWaterSensor() {
    return waterSensor;
}

void SensorManager::poll() {
    unsigned long now = millis();
    if (now - lastUpdateTime >= UPDATE_INTERVAL) {
        if (ultrasonicSensor) ultrasonicSensor->update();
        if (dhtSensor) dhtSensor->update();
        if (mq2Sensor) mq2Sensor->update();
        if (weightSensor) weightSensor->update();
        if (feederUltrasonic1) feederUltrasonic1->update();
        if (feederUltrasonic2) feederUltrasonic2->update();
        if (waterSensor) waterSensor->update();
        if (waterIRSensor) waterIRSensor->update();
        lastUpdateTime = now;
    }
}

// ===== MÉTODOS DEL ARENERO =====
float SensorManager::getLitterboxDistance() {
    if (ultrasonicSensor && ultrasonicSensor->isReady()) return ultrasonicSensor->getDistance();
    return -1.0f;
}

float SensorManager::getLitterboxTemperature() {
    if (dhtSensor && dhtSensor->isReady()) return dhtSensor->getTemperature();
    return -999.0f;
}

float SensorManager::getLitterboxHumidity() {
    if (dhtSensor && dhtSensor->isReady()) return dhtSensor->getHumidity();
    return -1.0f;
}

float SensorManager::getLitterboxGasPPM() {
    if (mq2Sensor && mq2Sensor->isReady()) return mq2Sensor->getPPM();
    return -1.0f;
}

LitterboxStepperMotor* SensorManager::getLitterboxMotor() {
    return litterboxMotor;
}

// ===== MÉTODOS DEL COMEDERO =====
float SensorManager::getFeederCatDistance() {
    if (feederUltrasonic1 && feederUltrasonic1->isReady()) return feederUltrasonic1->getDistance();
    return -1.0f;
}

float SensorManager::getFeederFoodDistance() {
    if (feederUltrasonic2 && feederUltrasonic2->isReady()) return feederUltrasonic2->getDistance();
    return -1.0f;
}

String SensorManager::getStorageFoodStatus() {
    if (feederUltrasonic2 && feederUltrasonic2->isReady()) {
        float d = feederUltrasonic2->getDistance();
        if (d <= 0) return "UNKNOWN";
        if (d <= 2.0f) return "FULL";
        if (d >= 13.0f) return "EMPTY";
        float pct = (13.0f - d) / (13.0f - 2.0f) * 100.0f;
        int ipct = (int) round(pct);
        if (ipct >= 45 && ipct <= 55) return "HALF";
        return "PARTIAL_" + String(ipct) + "%";
    }
    return "NOT_READY";
}

String SensorManager::getPlateFoodStatus() {
    if (feederUltrasonic1 && feederUltrasonic1->isReady()) {
        float d = feederUltrasonic1->getDistance();
        if (d <= 0) return "UNKNOWN";
        if (d <= 2.0f) return "FULL";
        if (d >= 8.0f) return "EMPTY";
        return "PARTIAL";
    }
    return "NOT_READY";
}

// ===== MÉTODOS DEL BEBEDERO =====
String SensorManager::getWaterLevel() {
    if (waterSensor && waterSensor->isReady()) {
        return waterSensor->getWaterLevel();
    }
    return "NOT_READY";
}

bool SensorManager::isWaterDetected() {
    if (waterSensor && waterSensor->isReady()) return waterSensor->isWaterDetected();
    return false;
}

bool SensorManager::isCatDrinking() {
    if (waterIRSensor && waterIRSensor->isReady()) return waterIRSensor->isObjectDetected();
    return false;
}

WaterDispenserPump* SensorManager::getWaterPump() {
    return waterPump;
}

// ===== ESTADO DE SENSORES =====
bool SensorManager::isLitterboxUltrasonicReady() { return ultrasonicSensor && ultrasonicSensor->isReady(); }
bool SensorManager::isLitterboxDHTReady()       { return dhtSensor && dhtSensor->isReady(); }
bool SensorManager::isLitterboxMQ2Ready()      { return mq2Sensor && mq2Sensor->isReady(); }
bool SensorManager::isFeederWeightReady()      { return weightSensor && weightSensor->isReady(); }
bool SensorManager::isFeederCatUltrasonicReady(){ return feederUltrasonic1 && feederUltrasonic1->isReady(); }
bool SensorManager::isFeederFoodUltrasonicReady(){ return feederUltrasonic2 && feederUltrasonic2->isReady(); }
bool SensorManager::isFeederMotorReady()       { return feederMotor && feederMotor->isReady(); }
bool SensorManager::isWaterLevelReady()        { return waterSensor && waterSensor->isReady(); }
bool SensorManager::isWaterIRReady()           { return waterIRSensor && waterIRSensor->isReady(); }

bool SensorManager::areAllSensorsReady() {
    return isLitterboxUltrasonicReady() && isLitterboxDHTReady() && isLitterboxMQ2Ready() &&
           isFeederWeightReady() && isFeederCatUltrasonicReady() && isFeederFoodUltrasonicReady() &&
           isWaterLevelReady() && isWaterIRReady();
}

String SensorManager::getSensorStatus() {
    String status = "{\"sensors\":{";
    status += "\"litterbox\":{";
    status += "\"ultrasonic\":{\"ready\":" + String(isLitterboxUltrasonicReady()) + "},";
    status += "\"dht\":{\"ready\":" + String(isLitterboxDHTReady()) + "},";
    status += "\"mq2\":{\"ready\":" + String(isLitterboxMQ2Ready()) + "}";
    status += "},";
    status += "\"feeder\":{";
    status += "\"weight\":{\"ready\":" + String(isFeederWeightReady()) + "},";
    status += "\"ultrasonic_cat\":{\"ready\":" + String(isFeederCatUltrasonicReady()) + "},";
    status += "\"ultrasonic_food\":{\"ready\":" + String(isFeederFoodUltrasonicReady()) + "},";
    status += "\"motor\":{\"ready\":" + String(isFeederMotorReady()) + "}";
    status += "},";
    status += "\"waterdispenser\":{";
    status += "\"water_sensor\":{\"ready\":" + String(isWaterLevelReady()) + "},";
    status += "\"pump\":{\"ready\":" + String(waterPump && waterPump->isReady()) + "},";
    status += "\"ir\":{\"ready\":" + String(isWaterIRReady()) + "}";
    status += "}";
    status += "}}";
    return status;
}

String SensorManager::getAllReadings() {
    String readings = "{\"readings\":{";
    readings += "\"litterbox\":{";
    readings += "\"distance\":" + String(getLitterboxDistance()) + ",";
    readings += "\"temperature\":" + String(getLitterboxTemperature()) + ",";
    readings += "\"humidity\":" + String(getLitterboxHumidity()) + ",";
    readings += "\"gas_ppm\":" + String(getLitterboxGasPPM());
    readings += "},";
    readings += "\"feeder\":{";
    readings += "\"weight\":" + String(getFeederWeight()) + ",";
    readings += "\"cat_distance\":" + String(getFeederCatDistance()) + ",";
    readings += "\"food_distance\":" + String(getFeederFoodDistance());
    readings += "},";
    readings += "\"waterdispenser\":{";
    readings += "\"water_level\":\"" + getWaterLevel() + "\",";
    readings += "\"cat_drinking\":" + String(isCatDrinking());
    readings += "}";
    readings += ",\"timestamp\":" + String(millis()) + "}}";
    return readings;
}

void SensorManager::printAllSensorReadings() {
    Serial.println(getAllReadings());
}
