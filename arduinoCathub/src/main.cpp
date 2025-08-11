// CatHub Arduino Firmware - IO Execution Layer
// This sketch delegates high-level logic to the Raspberry Pi. It exposes:
//  - Deterministic sensor reads on configurable intervals
//  - Motor / actuator execution commands
//  - Lightweight JSON line protocol over Serial (115200 baud)
//  - Dynamic configuration (intervals, thresholds, calibration)

#include <Arduino.h>
#include "state/ConfigStore.h"
#include "sensors/SensorManager.h"
#include "motors/MotorCoordinator.h"
#include "protocol/CommandProcessor.h"

ConfigStore configStore;
SensorManager sensorManager(configStore);
MotorCoordinator motorCoordinator(configStore);
CommandProcessor commandProcessor(configStore, sensorManager, motorCoordinator);

static unsigned long lastHousekeeping = 0;
const unsigned long HOUSEKEEPING_INTERVAL_MS = 1000UL; // 1 second

void setup() {
  Serial.begin(115200);
  while(!Serial) { /* wait for native USB (if any) */ }
  Serial.println(F("{\"event\":\"BOOT\",\"version\":\"0.1.0\"}"));

  configStore.loadDefaults();
  sensorManager.begin();
  motorCoordinator.begin();
  commandProcessor.begin();
}

void loop() {
  commandProcessor.poll();     // Process inbound serial commands
  sensorManager.poll();        // Run due sensor reads & emit data
  motorCoordinator.poll();     // Handle any staged motor operations

  const unsigned long now = millis();
  if (now - lastHousekeeping >= HOUSEKEEPING_INTERVAL_MS) {
    lastHousekeeping = now;
    if (configStore.shouldEmitHeartbeat()) {
      Serial.println(F("{\"event\":\"HEARTBEAT\"}"));
    }
  }
}