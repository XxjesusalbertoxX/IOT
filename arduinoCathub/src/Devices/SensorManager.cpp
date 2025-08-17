#include "SensorManager.h"
#include "feeder/config/SensorIDs.h"
#include "litterbox/config/SensorIDs.h"
#include "waterdispenser/config/SensorIDs.h"
#include "config/DeviceIDs.h" 

// ===== CONSTRUCTOR Y DESTRUCTOR =====
SensorManager::SensorManager() : initialized(false), lastUpdateTime(0) {
    // Sensores del arenero
    ultrasonicSensor = new LitterboxUltrasonicSensor(SENSOR_ID_LITTER_ULTRA, DEVICE_ID_LITTER);
    dhtSensor = new LitterboxDHTSensor(SENSOR_ID_LITTER_DHT, DEVICE_ID_LITTER);
    mq2Sensor = new LitterboxMQ2Sensor(SENSOR_ID_LITTER_MQ2, DEVICE_ID_LITTER);
    litterboxMotor = new LitterboxStepperMotor();

    // Sensores del comedero
    weightSensor = new FeederWeightSensor(SENSOR_ID_FEEDER_WEIGHT, DEVICE_ID_FEEDER);
    feederUltrasonic1 = new FeederUltrasonicSensor1(SENSOR_ID_FEEDER_SONIC1, DEVICE_ID_FEEDER);
    feederUltrasonic2 = new FeederUltrasonicSensor2(SENSOR_ID_FEEDER_SONIC2, DEVICE_ID_FEEDER);
    feederMotor = new FeederStepperMotor();

    // Sensores del bebedero
    waterSensor = new WaterDispenserSensor(SENSOR_ID_WATER_LEVEL, DEVICE_ID_WATER);
    waterIRSensor = new WaterDispenserIRSensor(SENSOR_ID_WATER_IR, DEVICE_ID_WATER);
    waterPump = new WaterDispenserPump();
}

SensorManager::~SensorManager() {
    // Arenero
    delete ultrasonicSensor;
    delete dhtSensor;
    delete mq2Sensor;
    delete litterboxMotor;
    // Comedero
    delete weightSensor;
    delete feederUltrasonic1;
    delete feederUltrasonic2;
    delete feederMotor;
    // Bebedero
    delete waterSensor;
    delete waterPump;
    delete waterIRSensor;
}

// ===== INICIALIZACIÓN =====
bool SensorManager::begin() {
    Serial.println("{\"sensor_manager\":\"INITIALIZING\"}");

    // Arenero
    bool ultrasonicOK = ultrasonicSensor->initialize();
    bool dhtOK = dhtSensor->initialize();
    bool mq2OK = mq2Sensor->initialize();

    // Comedero
    bool weightOK = weightSensor->initialize();
    bool feederUltrasonic1OK = feederUltrasonic1->initialize();
    bool feederUltrasonic2OK = feederUltrasonic2->initialize();
    bool feederMotorOK = feederMotor->initialize();

    // Bebedero
    bool waterSensorOK = waterSensor->initialize();
    bool waterPumpOK = waterPump->initialize();
    bool waterIROK = waterIRSensor->initialize();

    // Logs
    Serial.println("{\"sensor\":\"LITTERBOX_ULTRASONIC\",\"status\":\"" + String(ultrasonicOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"DHT\",\"status\":\"" + String(dhtOK ? "OK" : "FAILED") + "\"}");
    Serial.println("{\"sensor\":\"MQ2\",\"status\":\"" + String(mq2OK ? "OK" : "FAILED") + "\"}");
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

// ===== ACTUALIZACIÓN DE SENSORES =====
void SensorManager::poll() {
    unsigned long now = millis();
    if (now - lastUpdateTime >= UPDATE_INTERVAL) {
        // Arenero
        if (ultrasonicSensor) ultrasonicSensor->update();
        if (dhtSensor) dhtSensor->update();
        if (mq2Sensor) mq2Sensor->update();
        // Comedero
        if (weightSensor) weightSensor->update();
        if (feederUltrasonic1) feederUltrasonic1->update();
        if (feederUltrasonic2) feederUltrasonic2->update();
        // Bebedero
        if (waterSensor) waterSensor->update();
        if (waterPump) waterPump->update();
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
        return waterSensor->getWaterLevel(); // Esta función debe retornar un String descriptivo
    }
    return "NOT_READY";
}
bool SensorManager::isWaterDetected() {
    if (waterSensor && waterSensor->isReady()) return waterSensor->isWaterDetected();
    return false;
}
String SensorManager::getWaterLevelString() {
    if (waterSensor && waterSensor->isReady()) return waterSensor->getWaterLevel();
    return "NOT_READY";
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
bool SensorManager::isFeederUltrasonic1Ready() { return feederUltrasonic1 && feederUltrasonic1->isReady(); }
bool SensorManager::isFeederUltrasonic2Ready() { return feederUltrasonic2 && feederUltrasonic2->isReady(); }
bool SensorManager::isFeederMotorReady() { return feederMotor && feederMotor->isReady(); }
bool SensorManager::isWaterSensorReady() { return waterSensor && waterSensor->isReady(); }
bool SensorManager::isWaterPumpReady() { return waterPump && waterPump->isReady(); }
bool SensorManager::isWaterIRReady() { return waterIRSensor && waterIRSensor->isReady(); }
bool SensorManager::areAllSensorsReady() {
    return isLitterboxUltrasonicReady() && isLitterboxDHTReady() && isLitterboxMQ2Ready() &&
           isFeederWeightReady() && isFeederUltrasonic1Ready() && isFeederUltrasonic2Ready() && isFeederMotorReady() &&
           isWaterSensorReady() && isWaterPumpReady() && isWaterIRReady();
}

// ===== DIAGNÓSTICO =====
String SensorManager::getSensorStatus() {
    String status = "{\"sensors\":{";
    status += "\"litterbox\":{";
    status += "\"ultrasonic\":{\"ready\":" + String(isLitterboxUltrasonicReady()) + "},";
    status += "\"dht\":{\"ready\":" + String(isLitterboxDHTReady()) + "},";
    status += "\"mq2\":{\"ready\":" + String(isLitterboxMQ2Ready()) + "}";
    status += "},";
    status += "\"feeder\":{";
    status += "\"weight\":{\"ready\":" + String(isFeederWeightReady()) + "},";
    status += "\"ultrasonic_cat\":{\"ready\":" + String(isFeederUltrasonic1Ready()) + "},";
    status += "\"ultrasonic_food\":{\"ready\":" + String(isFeederUltrasonic2Ready()) + "},";
    status += "\"motor\":{\"ready\":" + String(isFeederMotorReady()) + "}";
    status += "},";
    status += "\"waterdispenser\":{";
    status += "\"water_sensor\":{\"ready\":" + String(isWaterSensorReady()) + "},";
    status += "\"pump\":{\"ready\":" + String(isWaterPumpReady()) + "},";
    status += "\"ir\":{\"ready\":" + String(isWaterIRReady()) + "}";
    status += "}";
    status += "}}";
    return status;
}

String SensorManager::getAllReadings() {
    String readings = "{\"readings\":{";
    // Arenero
    readings += "\"litterbox\":{";
    readings += "\"distance\":" + String(getLitterboxDistance()) + ",";
    readings += "\"temperature\":" + String(getLitterboxTemperature()) + ",";
    readings += "\"humidity\":" + String(getLitterboxHumidity()) + ",";
    readings += "\"gas_ppm\":" + String(getLitterboxGasPPM());
    readings += "},";
    // Comedero
    readings += "\"feeder\":{";
    readings += "\"weight_grams\":" + String(getFeederWeight()) + ",";
    readings += "\"cat_distance\":" + String(getFeederCatDistance()) + ",";
    readings += "\"food_distance\":" + String(getFeederFoodDistance());
    readings += "},";
    // Bebedero
    readings += "\"waterdispenser\":{";
    readings += "\"water_level\":" + String(getWaterLevel()) + ",";
    readings += "\"has_water\":" + String(isWaterDetected()) + ",";
    readings += "\"cat_drinking\":" + String(isCatDrinking()) + ",";
    readings += "\"level_string\":\"" + getWaterLevelString() + "\"";
    readings += "}";
    readings += ",\"timestamp\":" + String(millis()) + "}}";
    return readings;
}

void SensorManager::printAllSensorReadings() {
    // --- Litterbox ---
    Serial.print("{\"device_id\":\"");
    Serial.print(ultrasonicSensor->getDeviceId());
    Serial.print("\",\"sensor_id\":\"");
    Serial.print(ultrasonicSensor->getSensorId());
    Serial.print("\",\"value\":");
    Serial.print(ultrasonicSensor->getDistance());
    Serial.println("}");

    Serial.print("{\"device_id\":\"");
    Serial.print(dhtSensor->getDeviceId());
    Serial.print("\",\"sensor_id\":\"");
    Serial.print(dhtSensor->getSensorId());
    Serial.print("\",\"value\":");
    Serial.print(dhtSensor->getTemperature());
    Serial.println("}");

    Serial.print("{\"device_id\":\"");
    Serial.print(dhtSensor->getDeviceId());
    Serial.print("\",\"sensor_id\":\"");
    Serial.print(dhtSensor->getSensorId());
    Serial.print("_HUM\",\"value\":");
    Serial.print(dhtSensor->getHumidity());
    Serial.println("}");

    Serial.print("{\"device_id\":\"");
    Serial.print(mq2Sensor->getDeviceId());
    Serial.print("\",\"sensor_id\":\"");
    Serial.print(mq2Sensor->getSensorId());
    Serial.print("\",\"value\":");
    Serial.print(mq2Sensor->getPPM());
    Serial.println("}");

    // --- Feeder ---
    Serial.print("{\"device_id\":\"");
    Serial.print(weightSensor->getDeviceId());
    Serial.print("\",\"sensor_id\":\"");
    Serial.print(weightSensor->getSensorId());
    Serial.print("\",\"value\":");
    Serial.print(weightSensor->getCurrentWeight());
    Serial.println("}");

    Serial.print("{\"device_id\":\"");
    Serial.print(feederUltrasonic1->getDeviceId());
    Serial.print("\",\"sensor_id\":\"");
    Serial.print(feederUltrasonic1->getSensorId());
    Serial.print("\",\"value\":");
    Serial.print(feederUltrasonic1->getDistance());
    Serial.println("}");

    Serial.print("{\"device_id\":\"");
    Serial.print(feederUltrasonic2->getDeviceId());
    Serial.print("\",\"sensor_id\":\"");
    Serial.print(feederUltrasonic2->getSensorId());
    Serial.print("\",\"value\":");
    Serial.print(feederUltrasonic2->getDistance());
    Serial.println("}");

    // --- Waterdispenser ---
    Serial.print("{\"device_id\":\"");
    Serial.print(waterSensor->getDeviceId());
    Serial.print("\",\"sensor_id\":\"");
    Serial.print(waterSensor->getSensorId());
    Serial.print("\",\"value\":");
    Serial.print(waterSensor->getAnalogValue());
    Serial.println("}");

    Serial.print("{\"device_id\":\"");
    Serial.print(waterIRSensor->getDeviceId());
    Serial.print("\",\"sensor_id\":\"");
    Serial.print(waterIRSensor->getSensorId());
    Serial.print("\",\"value\":");
    Serial.print(waterIRSensor->isObjectDetected());
    Serial.println("}");
}