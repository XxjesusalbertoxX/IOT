// SensorManager.h
#pragma once
#include <Arduino.h>
#include <vector>
#include "../state/ConfigStore.h"
#include "BaseSensor.h"

// Forward declarations for simple stub sensors
class DummyAnalogSensor : public BaseSensor {
public:
	DummyAnalogSensor(const __FlashStringHelper *code, unsigned long &intervalRef, uint8_t pin)
		: BaseSensor(code, intervalRef), pin_(pin) { pinMode(pin_, INPUT); }
protected:
	void readAndPublish() override {
		int v = analogRead(pin_);
		Serial.print(F("{\"sensor\":\"")); Serial.print((const __FlashStringHelper*)code_); Serial.print(F("\",\"value\":")); Serial.print(v); Serial.println('}');
	}
private:
	uint8_t pin_;
};

class SensorManager {
public:
	explicit SensorManager(ConfigStore &cfg) : cfg_(cfg) {}
	void begin();
	void poll();
private:
	ConfigStore &cfg_;
	std::vector<BaseSensor*> sensors_;
};
