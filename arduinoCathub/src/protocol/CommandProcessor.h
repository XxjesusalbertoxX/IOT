#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include "../Devices/SensorManager.h"
#include "../Devices/litterbox/actuators/LitterboxStepperMotor.h"

static const float MIN_WEIGHT_GRAMS = 150.0f;
static const unsigned long WATER_PUMP_TIMEOUT = 5000;
static const unsigned long AUTO_CLEAN_COOLDOWN = 30000;

class CommandProcessor {
private:
    SensorManager* sensorManager;
    LitterboxStepperMotor* litterboxMotor;
    bool initialized;

    // ===== CONFIGURACIÓN DINÁMICA DE DISPOSITIVO =====
    String deviceIdentifier;
    String deviceType_str;
    bool deviceConfigured;

    static const int MAX_SENSORS = 5;
    String sensorIdentifiers[MAX_SENSORS];
    bool sensorConfigured[MAX_SENSORS];

    // ===== UMBRALES DEL ARENERO =====
    static constexpr float DEFAULT_CAT_DETECTION_DISTANCE = 14.0f;
    static constexpr float DEFAULT_CAT_PRESENT_THRESHOLD = 6.0f;
    static constexpr float DEFAULT_MAX_GAS_PPM = 800.0f;
    static constexpr float DEFAULT_MAX_HUMIDITY = 80.0f;
    static constexpr float DEFAULT_MIN_HUMIDITY = 15.0f;
    static constexpr float DEFAULT_MAX_TEMPERATURE = 35.0f;
    static constexpr float DEFAULT_MIN_TEMPERATURE = 10.0f;

    // Variables de umbrales (modificables)
    float catDetectionDistance;
    float catPresentThreshold;
    float maxGasPPM;
    float maxHumidity;
    float minHumidity;
    float maxTemperature;
    float minTemperature;
    bool thresholdsConfiguredByRaspberry;
    unsigned long lastConfigUpdate;
    unsigned long lastAutoCleanTime;

    // ===== VARIABLES DEL COMEDERO =====
    float maxWeightGrams;
    float minWeightGrams;
    float catEatingThreshold;
    float foodEmptyDistance;
    float foodFullDistance;
    bool feederThresholdsConfigured;
    bool feederAutoRefillEnabled;
    
    unsigned long lastFeederCheck;
    unsigned long feederMotorStartTime;
    bool feederMotorRunning;
    float lastWeightBeforeRefill;
    static const unsigned long FEEDER_TIMEOUT_MS = 30000;
    static const unsigned long CAT_DETECTION_INTERVAL = 5000;

    // ===== VARIABLES DEL WATER DISPENSER =====
    unsigned long lastWaterCheck;
    unsigned long waterPumpStartTime;
    bool waterPumpRunning;
    static const unsigned long WATER_CHECK_INTERVAL = 5000;
    static const unsigned long WATER_PUMP_TIMEOUT_MS = 10000;

    // ===== MÉTODOS DE CONFIGURACIÓN DINÁMICA =====
    void processDeviceCommand(String command, String params);
    void processSensorCommand(String command, String params);
    void sendAllSensorReadingsWithIdentifiers();
    
    // ===== MÉTODOS POR DISPOSITIVO =====
    void processLitterboxCommand(String command, String params);
    void processFeederCommand(String command, String params);
    void processWaterDispenserCommand(String command, String params);
    
    // ===== MÉTODOS DE UTILIDAD =====
    bool areAllSensorsReady();
    void processConfigCommand(String command, String params);
    void processStatusCommand();
    
    // ===== VALIDACIONES DE SEGURIDAD =====
    bool isLitterboxSafeToOperate();
    bool isLitterboxSafeToClean();
    bool isCatPresent();
    bool isLitterboxClean();
    String getValidationErrors();
    String getSafetyStatus();
    
    // ===== CONTROL DEL COMEDERO =====
    void updateFeederControl();
    bool isFeederNeedsRefill();
    bool isFeederSafeToOperate();
    bool isCatEating();
    bool hasFood();
    String getFeederStatus();
    void startAutoRefill();
    void stopAutoRefill();
    void checkFeederTimeout();
    void startFeederRefill();
    void stopFeederRefill();
    void setFeederThresholds(float minWeight, float maxWeight, float catThreshold, float emptyDistance, float fullDistance);
    
    // ===== CONTROL DEL WATER DISPENSER =====
    void updateWaterDispenserControl();
    
    // ===== GESTIÓN DE UMBRALES =====
    void initializeDefaultThresholds();
    void resetToDefaultThresholds();
    String getCurrentThresholds();
    void setThresholds(float catThreshold, float gasMax, float humMax, float humMin, float tempMax, float tempMin);
    bool areThresholdsConfigured() const;

public:
    CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* motor);
    bool initialize();
    void processCommand(String command);
    void update();
};

#endif