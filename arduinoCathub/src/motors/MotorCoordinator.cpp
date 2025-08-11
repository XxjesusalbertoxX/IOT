#include "MotorCoordinator.h"

void MotorCoordinator::begin() {
	pinMode(STEP1, OUTPUT); pinMode(DIR1, OUTPUT); pinMode(EN1, OUTPUT);
	pinMode(STEP2, OUTPUT); pinMode(DIR2, OUTPUT); pinMode(EN2, OUTPUT);
	pinMode(STEP3, OUTPUT); pinMode(DIR3, OUTPUT); pinMode(EN3, OUTPUT);
	digitalWrite(EN1, LOW); // enable (depende del driver, ajustar si es inverso)
	digitalWrite(EN2, LOW);
	digitalWrite(EN3, LOW);
	Serial.println(F("{\"event\":\"MOTOR_INIT\"}"));
}

bool MotorCoordinator::safetyBlocks() const {
	if (catPresent_) return true;
	if (catInside_) return true;
	if (gasAlert_) return true;
	if (action_.active) return false; // si ya está en marcha, no reevaluar hasta terminar
	// Si se solicita limpieza pero el compartimiento está demasiado sucio no iniciamos (depende de lógica externa)
	return false;
}

void MotorCoordinator::startRotationDegrees(int dir, int degrees, CylinderState targetStateAfter, bool multiPhase) {
	if (safetyBlocks()) {
		Serial.println(F("{\"warn\":\"BLOCKED_BY_SAFETY\"}"));
		return;
	}
	action_.active = true;
	action_.remainingSteps = degreesToSteps(degrees);
	action_.dir = dir; // dir relativo cilindro
	action_.phase = 0;
	// Set direction pins for both motors (inversos según configuración)
	int logicalDir = dir; // +1 derecha, -1 izquierda
	// Motor1
	digitalWrite(DIR1, (MOTOR1_DIR_NORMAL ? (logicalDir > 0) : (logicalDir < 0)) ? HIGH : LOW);
	// Motor2 (invertido físicamente)
	digitalWrite(DIR2, (MOTOR2_DIR_NORMAL ? (logicalDir > 0) : (logicalDir < 0)) ? HIGH : LOW);
	// Enable ambos durante movimiento
	digitalWrite(EN1, LOW);
	digitalWrite(EN2, LOW);
	// Guardar próximo estado final (cuando termine se asigna)
	// Para acciones multi-fase podríamos encadenar, aquí simplificado.
	state_ = targetStateAfter; // Asignamos al finalizar realmente (podríamos diferir)
}

void MotorCoordinator::completeAction() {
	action_.active = false;
	action_.remainingSteps = 0;
	Serial.print(F("{\"event\":\"CYL_ACTION_DONE\",\"state\":")); Serial.print(state_); Serial.println(F("}"));
}

void MotorCoordinator::stepBoth(int dir) {
	// Un solo pulso sincronizado
	digitalWrite(STEP1, HIGH);
	digitalWrite(STEP2, HIGH);
	delayMicroseconds(5);
	digitalWrite(STEP1, LOW);
	digitalWrite(STEP2, LOW);
	(void)dir; // dir ya aplicado en pines DIR
}

void MotorCoordinator::poll() {
	if (!action_.active) return;
	unsigned long nowMicros = micros();
	if (nowMicros - lastStepMicros_ < stepDelayMicros_) return;
	lastStepMicros_ = nowMicros;

	if (action_.remainingSteps > 0) {
		stepBoth(action_.dir);
		action_.remainingSteps--;
	} else {
		// Terminado
		completeAction();
	}
}

void MotorCoordinator::triggerLitterInserted() {
	if (state_ != CYL_SIN_ARENA || action_.active) return;
	// Giro izquierda 45º -> estado LIMPIO
	startRotationDegrees(-1, 45, CYL_LIMPIO);
}

void MotorCoordinator::requestCleaningCycle() {
	if (state_ != CYL_LIMPIO || action_.active) return;
	// 360º derecha -> vuelve a LIMPIO
	startRotationDegrees(1, 360, CYL_LIMPIO);
}

void MotorCoordinator::requestChangeLitter() {
	if (state_ != CYL_LIMPIO || action_.active) return;
	// 45º derecha -> luego vuelve a SIN_ARENA (simulación: 45º y regresar a 0 implicaría dos fases; aquí simple)
	startRotationDegrees(1, 45, CYL_SIN_ARENA);
}
