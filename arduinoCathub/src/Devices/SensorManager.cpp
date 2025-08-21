// SensorManager.cpp
#include "SensorManager.h"
#include "config/DeviceIDs.h"
#include "feeder/config/SensorIDs.h"
#include "feeder/config/ActuatorIDs.h"
#include "litterbox/config/SensorIDs.h"
#include "litterbox/config/ActuatorIDs.h"
#include "waterdispenser/config/SensorIDs.h"
#include "waterdispenser/config/ActuatorIDs.h"

// 🔥 NUEVO CONSTRUCTOR CON INYECCIÓN DE DEPENDENCIAS
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
    : initialized(false), lastUpdateTime(0) {
    
    // 🔥 ASIGNAR PUNTEROS EXTERNOS (NO CREAR NUEVOS OBJETOS)
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

// 🔥 DESTRUCTOR YA NO ELIMINA OBJETOS (SON EXTERNOS)
SensorManager::~SensorManager() {
    // Los objetos son externos, NO los eliminamos aquí
    // Solo resetear punteros por seguridad
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

    // 🔥 VERIFICAR QUE LOS PUNTEROS NO SEAN NULOS ANTES DE INICIALIZAR
    bool ultrasonicOK = (ultrasonicSensor != nullptr) ? ultrasonicSensor->initialize() : false;
    bool dhtOK = (dhtSensor != nullptr) ? dhtSensor->initialize() : false;
    bool mq2OK = (mq2Sensor != nullptr) ? mq2Sensor->initialize() : false;
    bool litterboxMotorOK = (litterboxMotor != nullptr) ? litterboxMotor->initialize() : false;
    
    bool weightOK = (weightSensor != nullptr) ? weightSensor->initialize() : false;
    bool feederUltrasonic1OK = (feederUltrasonic1 != nullptr) ? feederUltrasonic1->initialize() : false;
    bool feederUltrasonic2OK = (feederUltrasonic2 != nullptr) ? feederUltrasonic2->initialize() : false;
    bool feederMotorOK = (feederMotor != nullptr) ? feederMotor->initialize() : false;
    
    bool waterSensorOK = (waterSensor != nullptr) ? waterSensor->initialize() : false;
    bool waterPumpOK = (waterPump != nullptr) ? waterPump->initialize() : false;
    bool waterIROK = (waterIRSensor != nullptr) ? waterIRSensor->initialize() : false;

    Serial.println("{\"sensor\":\"LITTERBOX_ULTRASONIC\",\"status\":\"" + String(ultrasonicOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"DHT\",\"status\":\"" + String(dhtOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"MQ2\",\"status\":\"" + String(mq2OK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"LITTERBOX_MOTOR\",\"status\":\"" + String(litterboxMotorOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"FEEDER_WEIGHT\",\"status\":\"" + String(weightOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"FEEDER_ULTRASONIC_CAT\",\"status\":\"" + String(feederUltrasonic1OK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"FEEDER_ULTRASONIC_FOOD\",\"status\":\"" + String(feederUltrasonic2OK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"FEEDER_MOTOR\",\"status\":\"" + String(feederMotorOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"WATER_SENSOR\",\"status\":\"" + String(waterSensorOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"WATER_PUMP\",\"status\":\"" + String(waterPumpOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"WATER_IR\",\"status\":\"" + String(waterIROK ? "OK" : "FAILED") + "\"}");

    initialized = true;
    Serial.println("{\"sensor_manager\":\"READY\",\"all_systems\":\"INITIALIZED\"}");
    return initialized;
}

float SensorManager::getFeederWeight() {
    if (weightSensor && weightSensor->isReady()) return weightSensor->getCurrentWeight(); // ✅ CAMBIAR getWeight() por getCurrentWeight()
    return 0.0;
}

WaterDispenserSensor* SensorManager::getWaterSensor() {
    return waterSensor;
}

void SensorManager::poll() {
    unsigned long now = millis();
    if (now - lastUpdateTime >= UPDATE_INTERVAL) {
        // 🔥 VERIFICAR PUNTEROS ANTES DE USAR
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
    return -1.0;
}

float SensorManager::getLitterboxTemperature() {
    if (dhtSensor && dhtSensor->isReady()) return dhtSensor->getTemperature();
    return -999.0;
}

float SensorManager::getLitterboxHumidity() {
    if (dhtSensor && dhtSensor->isReady()) return dhtSensor->getHumidity();
    return -1.0;
}

float SensorManager::getLitterboxGasPPM() {
    if (mq2Sensor && mq2Sensor->isReady()) return mq2Sensor->getPPM();
    return -1.0;
}

LitterboxStepperMotor* SensorManager::getLitterboxMotor() {
    return litterboxMotor;
}

// ===== MÉTODOS DEL COMEDERO =====
float SensorManager::getFeederCatDistance() {
    if (feederUltrasonic1 && feederUltrasonic1->isReady()) return feederUltrasonic1->getDistance();
    return -1.0;
}

float SensorManager::getFeederFoodDistance() {
    if (feederUltrasonic2 && feederUltrasonic2->isReady()) return feederUltrasonic2->getDistance();
    return -1.0;
}

String SensorManager::getStorageFoodStatus() {
    if (feederUltrasonic2 && feederUltrasonic2->isReady()) {
        float d = feederUltrasonic2->getDistance();
        if (d <= 0) return "UNKNOWN";
        // Depósito: 2cm = full, 13cm = empty
        if (d <= 2.0) return "FULL";
        if (d >= 13.0) return "EMPTY";
        // parcial -> calcular porcentaje aproximado (100% en 2cm, 0% en 13cm)
        float pct = (13.0 - d) / (13.0 - 2.0) * 100.0; // 0..100
        int ipct = (int) round(pct);
        // Si está cerca del 50% informar HALF
        if (ipct >= 45 && ipct <= 55) return "HALF";
        // devolver parcial con porcentaje
        return "PARTIAL_" + String(ipct) + "%";
    }
    return "NOT_READY";
}

String SensorManager::getPlateFoodStatus() {
    if (feederUltrasonic1 && feederUltrasonic1->isReady()) {
        float d = feederUltrasonic1->getDistance();
        if (d <= 0) return "UNKNOWN";
        // Platito: <=2cm full, >=8cm empty
        if (d <= 2.0) return "FULL";
        if (d >= 8.0) return "EMPTY";
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
bool SensorManager::isLitterboxDHTReady() { return dhtSensor && dhtSensor->isReady(); }
bool SensorManager::isLitterboxMQ2Ready() { return mq2Sensor && mq2Sensor->isReady(); }
bool SensorManager::isFeederWeightReady() { return weightSensor && weightSensor->isReady(); }
bool SensorManager::isFeederCatUltrasonicReady() { return feederUltrasonic1 && feederUltrasonic1->isReady(); }
bool SensorManager::isFeederFoodUltrasonicReady() { return feederUltrasonic2 && feederUltrasonic2->isReady(); }
bool SensorManager::isFeederMotorReady() { return feederMotor && feederMotor->isReady(); }
bool SensorManager::isWaterLevelReady() { return waterSensor && waterSensor->isReady(); }
bool SensorManager::isWaterIRReady() { return waterIRSensor && waterIRSensor->isReady(); }

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