#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include "../Devices/SensorManager.h"
#include "../Devices/litterbox/actuators/LitterboxStepperMotor.h"
#include "../Devices/feeder/actuators/FeederStepperMotor.h"
#include "../Devices/waterdispenser/actuators/WaterDispenserPump.h"

class CommandProcessor {
private:
    SensorManager*           sensorManager;
    LitterboxStepperMotor*   litterboxMotor;
    FeederStepperMotor*      feederMotor;
    WaterDispenserPump*      waterPump;
    bool                     initialized;

    bool manualFeederControl;
    int  litterboxState; // 1 = INACTIVE, 2 = ACTIVE

    void processDeviceIDCommand(String command);

    // LTR1
    void sendLitterboxStatus();
    void setLitterboxReady();
    void startNormalCleaning();
    void startDeepCleaning();

    // feeder / water (sin cambios)
    void sendFeederStatus();
    void controlFeederMotor(bool on);

    void sendAllDevicesStatus();
    void sendPlainTextSensors();

    // seguridad
    bool isCatPresent();
    bool isLitterboxSafeToClean();
    bool isLitterboxSafeToOperate();
    bool isFeederSafeToOperate();
    bool hasSufficientFood();

public:
    CommandProcessor(SensorManager* sensors, LitterboxStepperMotor* litter,
                     FeederStepperMotor* feeder, WaterDispenserPump* water);

    bool initialize();
    void processCommand(String command);
    void update();
    

    int getLitterboxState() const { return litterboxState; }
};

#endif // COMMAND_PROCESSOR_H
