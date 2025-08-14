#ifndef LITTERBOX_STEPPER_MOTOR_H
#define LITTERBOX_STEPPER_MOTOR_H

#include <AccelStepper.h>
#include "../../config/MotorConfigs.h"

class LitterboxStepperMotor {
private:
  AccelStepper stepper;
  
  // Estado interno
  bool motorBlocked;
  bool motorEnabled;
  String currentState;  // "empty", "ready", "blocked"
  int cleaningIntervalMinutes;
  
  int degreesToSteps(float degrees);

public:
  LitterboxStepperMotor(int stepPin, int dirPin);
  
  void initialize();
  void update();
  
  // Métodos legibles y claros
  bool moveToPosition(float degrees);
  bool setBlocked(bool blocked);
  bool setEnabled(bool enabled);
  bool setState(const String& state);
  bool setCleaningInterval(int minutes);
  
  // Getters
  String getState() const { return currentState; }
  bool isBlocked() const { return motorBlocked; }
  bool isEnabled() const { return motorEnabled; }
  int getCleaningInterval() const { return cleaningIntervalMinutes; }
  
  void emergencyStop();
};

#endif