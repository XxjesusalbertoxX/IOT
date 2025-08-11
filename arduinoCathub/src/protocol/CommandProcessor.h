// CommandProcessor.h
#pragma once
#include <Arduino.h>
#include "../state/ConfigStore.h"
#include "../sensors/SensorManager.h"
#include "../motors/MotorCoordinator.h"

class CommandProcessor {
public:
	CommandProcessor(ConfigStore &cfg, SensorManager &sm, MotorCoordinator &mc)
		: cfg_(cfg), sm_(sm), mc_(mc) {}
	void begin();
	void poll();
private:
	void processLine(char *line);
	ConfigStore &cfg_;
	SensorManager &sm_;
	MotorCoordinator &mc_;
	static const size_t BUF_SZ = 128;
	char buf_[BUF_SZ];
	size_t idx_ = 0;
};
