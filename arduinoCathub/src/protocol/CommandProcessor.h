// CommandProcessor.h
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
    bool manualFeederControl;     // Control manual del comedero (botón presionado)
    int litterboxState;
    
    // ===== MÉTODOS PRINCIPALES =====
    void processDeviceIDCommand(String command);
    
    // ===== COMANDOS LTR1 (ARENERO) =====
    void sendLitterboxStatus();
    void setLitterboxReady();
    void startNormalCleaning();
    void startDeepCleaning();
    void setLitterboxCleaningInterval(int minutes);
    
    // ===== COMANDOS FDR1 (COMEDERO) =====
    void sendFeederStatus();
    void controlFeederMotor(bool on);
    
    // ===== COMANDO ALL =====
    void sendAllDevicesStatus();
    
    // ===== VALIDACIONES DE SEGURIDAD =====
    bool isCatPresent();
    bool isLitterboxSafeToOperate();
    bool isLitterboxSafeToClean();
    bool isFeederSafeToOperate();
    bool hasSufficientFood();

public:
    CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* litter, 
                    FeederStepperMotor* feeder, WaterDispenserPump* water);
    bool initialize();
    void processCommand(String command);
    void update();
    
    // ===== GETTERS - SOLO LOS NECESARIOS =====
    int getLitterboxState() const { return litterboxState; }
    
    // 🔥 ELIMINAR ESTOS GETTERS QUE CAUSAN ERROR:
    // bool isFeederEnabled() const { return feederEnabled; }  ❌ ELIMINAR
    // int getTargetWeight() const { return targetWeight; }    ❌ ELIMINAR
};

#endif