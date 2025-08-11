#include "SensorManager.h"

// Example pin assignments (adjust as needed)
static const uint8_t PIN_GAS = A0;
static const uint8_t PIN_WEIGHT_DOUT = 48; // placeholder, real HX711 uses separate lib (future)

void SensorManager::begin() {
	// Create stub sensors (replace with concrete specialized sensor classes)
	sensors_.push_back(new DummyAnalogSensor(F("gas"), cfg_.mutableIntervals().gasMs, PIN_GAS));
	sensors_.push_back(new DummyAnalogSensor(F("peso_raw"), cfg_.mutableIntervals().pesoMs, PIN_WEIGHT_DOUT));
}

void SensorManager::poll() {
	unsigned long now = millis();
	for (auto *s : sensors_) {
		s->poll(now);
	}
}
