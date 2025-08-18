#include "SensorManager.h"

SensorManager::SensorManager() : initialized(false), lastUpdateTime(0) {
    // Inicializar sensores sin IDs hardcodeados
    ultrasonicSensor = new LitterboxUltrasonicSensor(nullptr, nullptr);
    dhtSensor = new LitterboxDHTSensor(nullptr, nullptr);
    mq2Sensor = new LitterboxMQ2Sensor(nullptr, nullptr);
    litterboxMotor = new LitterboxStepperMotor();
    
    weightSensor = new FeederWeightSensor(nullptr, nullptr);
    feederUltrasonic1 = new FeederUltrasonicSensor1(nullptr, nullptr);
    feederUltrasonic2 = new FeederUltrasonicSensor2(nullptr, nullptr);
    feederMotor = new FeederStepperMotor();
    
    waterSensor = new WaterDispenserSensor(nullptr, nullptr);
    waterIRSensor = new WaterDispenserIRSensor(nullptr, nullptr);
    waterPump = new WaterDispenserPump();
}

SensorManager::~SensorManager() {
    delete ultrasonicSensor;
    delete dhtSensor;
    delete mq2Sensor;
    delete litterboxMotor;
    delete weightSensor;
    delete feederUltrasonic1;
    delete feederUltrasonic2;
    delete feederMotor;
    delete waterSensor;
    delete waterPump;
    delete waterIRSensor;
}

bool SensorManager::begin() {
    Serial.println("{\"sensor_manager\":\"INITIALIZING\"}");

    bool ultrasonicOK = ultrasonicSensor->initialize();
    bool dhtOK = dhtSensor->initialize();
    bool mq2OK = mq2Sensor->initialize();
    bool litterboxMotorOK = litterboxMotor->initialize();
    
    bool weightOK = weightSensor->initialize();
    bool feederUltrasonic1OK = feederUltrasonic1->initialize();
    bool feederUltrasonic2OK = feederUltrasonic2->initialize();
    bool feederMotorOK = feederMotor->initialize();
    
    bool waterSensorOK = waterSensor->initialize();
    bool waterPumpOK = waterPump->initialize();
    bool waterIROK = waterIRSensor->initialize();

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
float SensorManager::getFeederWeight() {
    if (weightSensor && weightSensor->isReady()) return weightSensor->getCurrentWeight();
    return -1.0;
}

float SensorManager::getFeederCatDistance() {
    if (feederUltrasonic1 && feederUltrasonic1->isReady()) return feederUltrasonic1->getDistance();
    return -1.0;
}

float SensorManager::getFeederFoodDistance() {
    if (feederUltrasonic2 && feederUltrasonic2->isReady()) return feederUltrasonic2->getDistance();
    return -1.0;
}

FeederStepperMotor* SensorManager::getFeederMotor() {
    return feederMotor;
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

// ✅ IMPLEMENTAR MÉTODO FALTANTE
void SensorManager::printAllSensorReadings() {
    Serial.println(getAllReadings());
}