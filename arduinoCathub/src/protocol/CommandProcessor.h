#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include "../sensors/SensorManager.h"
#include "../sensors/litterbox/LitterboxStepperMotor.h"

class CommandProcessor {
private:
    SensorManager* sensorManager;
    LitterboxStepperMotor* litterboxMotor;
    
    bool initialized;

    // ✅ CONFIGURACIÓN DINÁMICA DE DISPOSITIVO
    static const int MAX_SENSORS = 5;  // Máximo 5 sensores por dispositivo
    String deviceIdentifier;           // ej: "ARENERO-001"  
    String deviceType_str;             // ej: "litterbox"
    bool deviceConfigured;
    
    String sensorIdentifiers[MAX_SENSORS];  // ej: ["TEMP_ARENERO-001", "ULTRA_ARENERO-001", ...]
    bool sensorConfigured[MAX_SENSORS];     // true si el sensor está configurado

    // ===== WATER DISPENSER =====
    unsigned long lastWaterCheck = 0;
    unsigned long waterPumpStartTime = 0;
    bool waterPumpRunning = false;
    static const unsigned long WATER_CHECK_INTERVAL = 5000;
    static const unsigned long WATER_PUMP_TIMEOUT = 10000;

    // ===== UMBRALES DEL ARENERO =====
    static constexpr float DEFAULT_CAT_DETECTION_DISTANCE = 14.0f;
    static constexpr float DEFAULT_CAT_PRESENT_THRESHOLD = 6.0f;
    static constexpr float DEFAULT_MAX_GAS_PPM = 800.0f;
    static constexpr float DEFAULT_MAX_HUMIDITY = 80.0f;
    static constexpr float DEFAULT_MIN_HUMIDITY = 15.0f;
    static constexpr float DEFAULT_MAX_TEMPERATURE = 35.0f;
    static constexpr float DEFAULT_MIN_TEMPERATURE = 10.0f;

    // Variables de umbrales (modificables desde Raspberry Pi)
    float catDetectionDistance;
    float catPresentThreshold;
    float maxGasPPM;
    float maxHumidity;
    float minHumidity;
    float maxTemperature;
    float minTemperature;
    bool thresholdsConfiguredByRaspberry;
    unsigned long lastConfigUpdate;

    // ===== UMBRALES DEL COMEDERO =====
    static constexpr float DEFAULT_MIN_WEIGHT = 150.0f;
    static constexpr float DEFAULT_MAX_WEIGHT = 500.0f;
    static constexpr float DEFAULT_CAT_EATING_THRESHOLD = 15.0f;
    static constexpr float DEFAULT_FOOD_EMPTY_DISTANCE = 8.0f;
    static constexpr float DEFAULT_FOOD_FULL_DISTANCE = 2.0f;

    // Variables del comedero
    static constexpr float MIN_WEIGHT_GRAMS = 150.0f;
    float maxWeightGrams = 500.0f;
    unsigned long lastFeederCheck = 0;
    unsigned long feederMotorStartTime = 0;
    bool feederMotorRunning = false;
    float lastWeightBeforeRefill = 0.0;
    static const unsigned long FEEDER_TIMEOUT_MS = 30000;
    static const unsigned long CAT_DETECTION_INTERVAL = 5000;
    
    // ✅ MÉTODOS DE CONFIGURACIÓN DINÁMICA
    void processDeviceCommand(String command, String params);
    void sendAllSensorReadingsWithIdentifiers();
    void sendAllSensorIdentifiers();
    bool getAllSensorsConfigured();
    
    // ===== MÉTODOS POR DISPOSITIVO =====
    void processLitterboxCommand(String command, String params);
    void processFeederCommand(String command, String params);
    void processWaterDispenserCommand(String command, String params);
    void processSensorCommand(String command, String params);
    void processConfigCommand(String command, String params);
    void processStatusCommand();
    
    // ===== VALIDACIONES DE SEGURIDAD ARENERO =====
    bool isLitterboxSafeToOperate();
    bool isLitterboxSafeToClean();
    bool isCatPresent();
    bool isLitterboxClean();
    String getValidationErrors();
    String getSafetyStatus();
    void sendAllSensorReadings();
    bool areAllSensorsReady();
    
    // ===== CONTROL DEL COMEDERO =====
    void updateFeederControl();
    bool isFeederNeedsRefill();
    bool isCatEating();
    bool hasFood();
    String getFeederStatus();
    void startAutoRefill();
    void stopAutoRefill();
    void checkFeederTimeout();
    
    // ===== CONTROL DEL WATER DISPENSER =====
    String getWaterDispenserStatus();
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
    void update(); // Verificaciones continuas + control automático
};

#endif