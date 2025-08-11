#include "ConfigStore.h"

void ConfigStore::loadDefaults() {
	// Defaults already set in struct initializers
}

bool ConfigStore::setInterval(const String &key, unsigned long ms) {
	if (ms < 100) return false; // basic sanity
	if (key == F("comida")) intervals_.comidaMs = ms;
	else if (key == F("agua")) intervals_.aguaMs = ms;
	else if (key == F("peso")) intervals_.pesoMs = ms;
	else if (key == F("gas")) intervals_.gasMs = ms;
	else if (key == F("temphum")) intervals_.tempHumMs = ms;
	else if (key == F("presencia")) intervals_.presenciaMs = ms;
	else return false;
	return true;
}

bool ConfigStore::setThreshold(const String &key, float value) {
	if (key == F("peso_dispensar_gr")) thresholds_.pesoDispensarGr = value;
	else if (key == F("gas_ppm_warning")) thresholds_.gasPpmWarning = value;
	else return false;
	return true;
}
