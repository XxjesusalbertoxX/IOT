// ConfigStore.h
#pragma once
#include <Arduino.h>

struct IntervalConfig {
	unsigned long comidaMs = 30000UL;     // food level sensor interval
	unsigned long aguaMs = 30000UL;       // water level sensor interval
	unsigned long pesoMs = 10000UL;       // weight sensor interval
	unsigned long gasMs = 30000UL;        // gas sensor interval
	unsigned long tempHumMs = 900000UL;   // 15 min
	unsigned long presenciaMs = 5000UL;   // presence / distance sensors
};

struct ThresholdConfig {
	float pesoDispensarGr = 50.0f; // portion weight target
	float gasPpmWarning = 600.0f;
};

class ConfigStore {
public:
	void loadDefaults();
	bool shouldEmitHeartbeat() const { return heartbeat_; }

	// Accessors
	const IntervalConfig &intervals() const { return intervals_; }
	IntervalConfig &mutableIntervals() { return intervals_; }
	ThresholdConfig &thresholds() { return thresholds_; }
	const ThresholdConfig &getThresholds() const { return thresholds_; }

	// Update helpers
	bool setInterval(const String &key, unsigned long ms);
	bool setThreshold(const String &key, float value);
	void setHeartbeat(bool on) { heartbeat_ = on; }

private:
	IntervalConfig intervals_{};
	ThresholdConfig thresholds_{};
	bool heartbeat_ = true;
};
