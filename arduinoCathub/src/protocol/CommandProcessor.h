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

    unsigned long lastWaterCheck = 0;
    unsigned long waterPumpStartTime = 0;
    bool waterPumpRunning = false;
    static const unsigned long WATER_CHECK_INTERVAL = 5000;    // 5 segundos
    static const unsigned long WATER_PUMP_TIMEOUT = 10000;     // 10 segundos máximo

    // ===== UMBRALES DEL ARENERO (valores por defecto seguros) =====
    static constexpr float DEFAULT_CAT_DETECTION_DISTANCE = 14.0f;
    static constexpr float DEFAULT_CAT_PRESENT_THRESHOLD = 6.0f;
    static constexpr float DEFAULT_MAX_GAS_PPM = 800.0f;
    static constexpr float DEFAULT_MAX_HUMIDITY = 80.0f;
    static constexpr float DEFAULT_MIN_HUMIDITY = 15.0f;
    static constexpr float DEFAULT_MAX_TEMPERATURE = 35.0f;
    static constexpr float DEFAULT_MIN_TEMPERATURE = 10.0f;

    // ===== UMBRALES DEL COMEDERO (valores por defecto) =====
    static constexpr float DEFAULT_MIN_WEIGHT = 150.0f;     // Gramos mínimos
    static constexpr float DEFAULT_MAX_WEIGHT = 500.0f;     // Gramos máximos por defecto
    static constexpr float DEFAULT_CAT_EATING_THRESHOLD = 15.0f; // cm para detectar gato comiendo
    static constexpr float DEFAULT_FOOD_EMPTY_DISTANCE = 8.0f;   // cm cuando el compartimento está vacío
    static constexpr float DEFAULT_FOOD_FULL_DISTANCE = 2.0f;    // cm cuando está lleno

    // 🛠️ UMBRALES ACTUALES DEL ARENERO (modificables desde Raspberry Pi)
    float catDetectionDistance;
    float catPresentThreshold;
    float maxGasPPM;
    float maxHumidity;
    float minHumidity;
    float maxTemperature;
    float minTemperature;
    bool thresholdsConfiguredByRaspberry;
    unsigned long lastConfigUpdate;

        // ...existing code...
    
    private:
        // ===== COMEDERO SIMPLIFICADO =====
        static constexpr float MIN_WEIGHT_GRAMS = 150.0f;  // FIJO: 150g mínimos
        float maxWeightGrams = 500.0f;                      // VARIABLE: viene de la Raspberry Pi
        
        // Control automático
        unsigned long lastFeederCheck = 0;
        unsigned long feederMotorStartTime = 0;
        bool feederMotorRunning = false;
        float lastWeightBeforeRefill = 0.0;
        
        static const unsigned long FEEDER_TIMEOUT_MS = 30000;      // 30 segundos máximo
        static const unsigned long CAT_DETECTION_INTERVAL = 5000;  // 5 segundos
        
        // Métodos simplificados
        void processFeederCommand(String command, String params);
        void updateFeederControl();                 // Solo auto-rellenado
        bool isFeederNeedsRefill();                // Peso < 150g
        bool isCatEating();                        // Gato detectado cada 5s
        bool hasFood();                            // Sensor de comida disponible
        String getFeederStatus();
        void startAutoRefill();                    // Auto-rellenado
        void stopAutoRefill();
        void checkFeederTimeout();
    
    // ===== VALIDACIONES DE SEGURIDAD DEL ARENERO =====
    bool isLitterboxSafeToOperate();
    bool isLitterboxSafeToClean();
    bool isCatPresent();
    bool isLitterboxClean();
    String getValidationErrors();
    String getSafetyStatus();
    void sendAllSensorReadings();
    bool areAllSensorsReady();
    
    // ===== CONTROL INTELIGENTE DEL COMEDERO =====
    void updateFeederControl();
    bool isFeederNeedsRefill();
    bool isCatEating();
    bool hasFood();
    bool isFeederSafeToOperate();
    String getFeederStatus();
    void startFeederRefill();
    void stopFeederRefill();
    void checkFeederTimeout();
    
    // ===== GESTIÓN DE UMBRALES =====
    void initializeDefaultThresholds();
    void resetToDefaultThresholds();
    String getCurrentThresholds();
    void setThresholds(float catThreshold, float gasMax, float humMax, float humMin, float tempMax, float tempMin);
    void setFeederThresholds(float minWeight, float maxWeight, float catThreshold, float emptyDistance, float fullDistance);
    bool areThresholdsConfigured() const;

public:
    CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* motor);
    bool initialize();
    void processCommand(String command);
    void update(); // Verificaciones de seguridad continuas + control del comedero
};

#endif