#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include "../Devices/SensorManager.h"
#include "../Devices/litterbox/actuators/LitterboxStepperMotor.h"
#include "../Devices/feeder/actuators/FeederStepperMotor.h"
#include "../Devices/waterdispenser/actuators/WaterDispenserPump.h"

class CommandProcessor {
private:
    SensorManager* sensorManager;
    LitterboxStepperMotor* litterboxMotor;
    FeederStepperMotor* feederMotor;
    WaterDispenserPump* waterPump;
    bool initialized;
    
    // ===== ESTADOS DE DISPOSITIVOS =====
    bool feederEnabled;           // Estado de activación del comedero
    int targetWeight;             // Peso objetivo en gramos para rellenar
    bool manualFeederControl;     // Control manual del comedero (botón presionado)
    unsigned long lastFeederRetry;// Último intento de rellenar automático
    int litterboxState;           // 1=ready, 2.1=cleaning_normal, 2.2=cleaning_deep
    
    // ===== MÉTODOS PRINCIPALES =====
    void processDeviceIDCommand(String command);
    
    // ===== COMANDOS LTR1 (ARENERO) =====
    void sendLitterboxStatus();
    void setLitterboxReady();
    void startNormalCleaning();
    void startDeepCleaning();
    
    // ===== COMANDOS FDR1 (COMEDERO) =====
    void sendFeederStatus();
    void setTargetWeight(int grams);
    void controlFeederMotor(bool on);
    void attemptFeederRefill();
    
    // ===== COMANDO ALL =====
    void sendAllDevicesStatus();
    
    // ===== VALIDACIONES DE SEGURIDAD =====
    bool isCatPresent();
    bool isLitterboxSafeToClean();
    bool isFeederSafeToOperate();
    bool hasSufficientFood();

public:
    CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* litter, 
                    FeederStepperMotor* feeder, WaterDispenserPump* water);
    bool initialize();
    void processCommand(String command);
    void update();
    
    // ===== GETTERS =====
    bool isFeederEnabled() const { return feederEnabled; }
    int getTargetWeight() const { return targetWeight; }
    int getLitterboxState() const { return litterboxState; }
};

#endif