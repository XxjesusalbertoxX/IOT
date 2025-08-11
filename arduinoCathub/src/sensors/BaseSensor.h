// BaseSensor.h
#pragma once
#include <Arduino.h>

class BaseSensor {
public:
	BaseSensor(const __FlashStringHelper *code, unsigned long &intervalRef)
		: code_(code), intervalRef_(intervalRef) {}
	virtual ~BaseSensor() = default;

	void poll(unsigned long now) {
		if (now - lastRun_ >= intervalRef_) {
			lastRun_ = now;
			readAndPublish();
		}
	}

protected:
	virtual void readAndPublish() = 0; // implement measurement + Serial println JSON
	const __FlashStringHelper *code_;
	unsigned long &intervalRef_;
	unsigned long lastRun_ = 0;
};
