#include "CommandProcessor.h"

void CommandProcessor::begin() {
	idx_ = 0;
}

void CommandProcessor::poll() {
	while (Serial.available()) {
		char c = (char)Serial.read();
		if (c == '\n' || c == '\r') {
			if (idx_ > 0) {
				buf_[idx_] = '\0';
				processLine(buf_);
				idx_ = 0;
			}
		} else if (idx_ < BUF_SZ - 1) {
			buf_[idx_++] = c;
		}
	}
}

void CommandProcessor::processLine(char *line) {
	// Simple space-delimited commands
	// Examples:
	//   SET_INTERVAL peso 15000
	//   SET_THRESHOLD peso_dispensar_gr 55.5
	//   MOTOR 0 1 200 (id dir steps)
	//   HEARTBEAT 0
	//   PING
	char *cmd = strtok(line, " ");
	if (!cmd) return;
	if (!strcmp(cmd, "PING")) {
		Serial.println(F("{\"pong\":true}"));
	} else if (!strcmp(cmd, "SET_INTERVAL")) {
		char *k = strtok(NULL, " ");
		char *v = strtok(NULL, " ");
		if (k && v) {
			unsigned long ms = strtoul(v, nullptr, 10);
			bool ok = cfg_.setInterval(String(k), ms);
			Serial.print(F("{\"ack\":\"SET_INTERVAL\",\"ok\":")); Serial.print(ok?F("true"):F("false")); Serial.println(F("}"));
		}
	} else if (!strcmp(cmd, "SET_THRESHOLD")) {
		char *k = strtok(NULL, " ");
		char *v = strtok(NULL, " ");
		if (k && v) {
			float f = atof(v);
			bool ok = cfg_.setThreshold(String(k), f);
			Serial.print(F("{\"ack\":\"SET_THRESHOLD\",\"ok\":")); Serial.print(ok?F("true"):F("false")); Serial.println(F("}"));
		}
	} else if (!strcmp(cmd, "MOTOR")) {
		char *id = strtok(NULL, " ");
		char *dir = strtok(NULL, " ");
		char *steps = strtok(NULL, " ");
		if (id && dir && steps) {
			MotorCommand mc{ (uint8_t)atoi(id), atol(steps), atoi(dir) >= 0 ? 1 : -1 };
			bool ok = mc_.enqueue(mc);
			Serial.print(F("{\"ack\":\"MOTOR\",\"ok\":")); Serial.print(ok?F("true"):F("false")); Serial.println(F("}"));
		}
	} else if (!strcmp(cmd, "HEARTBEAT")) {
		char *v = strtok(NULL, " ");
		if (v) cfg_.setHeartbeat(atoi(v) != 0);
		Serial.println(F("{\"ack\":\"HEARTBEAT\"}"));
	} else {
		Serial.print(F("{\"error\":\"UNKNOWN_CMD\",\"cmd\":\"")); Serial.print(cmd); Serial.println(F("\"}"));
	}
}
