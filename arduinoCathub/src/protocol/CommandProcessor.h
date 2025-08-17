#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include "../Devices/SensorManager.h"
#include "../Devices/litterbox/actuators/LitterboxStepperMotor.h"

class CommandProcessor {
public:
    // Constructores y estado
    CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* motor);
    bool initialize();
    bool initialized = false;

    // Acceso a hardware principal
    SensorManager* sensorManager;
    LitterboxStepperMotor* litterboxMotor;

    // ===== UMBRALES DEL ARENERO (valores por defecto seguros) =====
    static constexpr float DEFAULT_CAT_DETECTION_DISTANCE = 14.0f;
    static constexpr float DEFAULT_CAT_PRESENT_THRESHOLD = 6.0f;
    static constexpr float DEFAULT_MAX_GAS_PPM = 800.0f;
    static constexpr float DEFAULT_MAX_HUMIDITY = 80.0f;
    static constexpr float DEFAULT_MIN_HUMIDITY = 15.0f;
    static constexpr float DEFAULT_MAX_TEMPERATURE = 35.0f;
    static constexpr float DEFAULT_MIN_TEMPERATURE = 10.0f;

    // ===== UMBRALES DEL COMEDERO (valores por defecto) =====
    static constexpr float DEFAULT_MIN_WEIGHT = 150.0f;
    static constexpr float DEFAULT_MAX_WEIGHT = 500.0f;
    static constexpr float DEFAULT_CAT_EATING_THRESHOLD = 15.0f;
    static constexpr float DEFAULT_FOOD_EMPTY_DISTANCE = 8.0f;
    static constexpr float DEFAULT_FOOD_FULL_DISTANCE = 2.0f;

    // ===== UMBRALES ACTUALES DEL ARENERO (modificables desde Raspberry Pi) =====
    float catDetectionDistance;
    float catPresentThreshold;
    float maxGasPPM;
    float maxHumidity;
    float minHumidity;
    float maxTemperature;
    float minTemperature;
    bool thresholdsConfiguredByRaspberry = false;
    unsigned long lastConfigUpdate = 0;

    // ===== CONTROL DE AGUA =====
    unsigned long lastWaterCheck = 0;
    unsigned long waterPumpStartTime = 0;
    bool waterPumpRunning = false;
    static const unsigned long WATER_CHECK_INTERVAL = 5000;
    static const unsigned long WATER_PUMP_TIMEOUT = 10000;

    // ===== MÉTODOS PRINCIPALES =====
    void processCommand(String command);
    void update(); // Verificaciones de seguridad continuas + control del comedero

    // ===== VALIDACIONES DE SEGURIDAD DEL ARENERO =====
    void processLitterboxCommand(String command, String params);
    void processSensorCommand(String command, String params);
    void processConfigCommand(String command, String params);
    void processStatusCommand();
    bool isLitterboxSafeToOperate();
    bool isLitterboxSafeToClean();
    bool isCatPresent();
    bool isLitterboxClean();
    String getValidationErrors();
    String getSafetyStatus();
    void sendAllSensorReadings();
    bool areAllSensorsReady();

    // ===== CONTROL INTELIGENTE DEL COMEDERO =====
    bool isFeederSafeToOperate();

    // ===== GESTIÓN DE UMBRALES =====
    void initializeDefaultThresholds();
    void resetToDefaultThresholds();
    String getCurrentThresholds();
    void setThresholds(float catThreshold, float gasMax, float humMax, float humMin, float tempMax, float tempMin);
    void setFeederThresholds(float minWeight, float maxWeight, float catThreshold, float emptyDistance, float fullDistance);
    bool areThresholdsConfigured() const;

    // ===== COMEDERO: CONTROL Y ESTADO =====
    void processFeederCommand(String command, String params);
    void updateFeederControl();
    bool isFeederNeedsRefill();
    bool isCatEating();
    bool hasFood();
    String getFeederStatus();
    void startAutoRefill();
    void stopAutoRefill();
    void checkFeederTimeout();
    void startFeederRefill();
    void stopFeederRefill();

    // ===== CONTROL INTELIGENTE DEL DISPENSADOR DE AGUA =====
    void updateWaterDispenserControl();

private:
    // ===== COMEDERO: VARIABLES DE CONTROL =====
    static constexpr float MIN_WEIGHT_GRAMS = 150.0f;
    float maxWeightGrams = 500.0f;
    float minWeightGrams = 150.0f;
    float catEatingThreshold = 15.0f;
    float foodEmptyDistance = 8.0f;
    float foodFullDistance = 2.0f;
    bool feederThresholdsConfigured = false;

    unsigned long lastFeederCheck = 0;
    unsigned long feederMotorStartTime = 0;
    bool feederMotorRunning = false;
    float lastWeightBeforeRefill = 0.0f;

    bool feederAutoRefillEnabled = false;
    static const unsigned long FEEDER_TIMEOUT_MS = 30000;
    static const unsigned long CAT_DETECTION_INTERVAL = 5000;
};

#endif