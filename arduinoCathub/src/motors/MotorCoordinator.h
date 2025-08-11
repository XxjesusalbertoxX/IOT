// MotorCoordinator.h
#pragma once
#include <Arduino.h>
#include "../state/ConfigStore.h"

// Estados de la máquina del cilindro (motores 1 y 2 sincronizados)
enum CylinderState : uint8_t {
	CYL_SIN_ARENA = 0,
	CYL_LIMPIO    = 1,
	CYL_LIMPIANDO = 2,
	CYL_CAMBIO_ARENA = 3
};

struct CylinderAction {
	bool active = false;
	long remainingSteps = 0;
	int dir = 1; // +1 derecha, -1 izquierda (referencia cilindro)
	uint8_t phase = 0; // para acciones que tienen varias fases
};

class MotorCoordinator {
public:
	explicit MotorCoordinator(ConfigStore &cfg) : cfg_(cfg) {}
	void begin();
	void poll();

	// Triggers externos (llamados desde CommandProcessor futuramente)
	void triggerLitterInserted();      // Pasa de SIN_ARENA -> LIMPIO (giro izquierda 45º)
	void requestCleaningCycle();       // LIMPIO -> LIMPIANDO (360º derecha)
	void requestChangeLitter();        // LIMPIO -> CAMBIO_ARENA (45º derecha)

	// Flags de seguridad (sensores externos actualizarán esto):
	void setCatPresent(bool v) { catPresent_ = v; }
	void setCatInsideLitter(bool v) { catInside_ = v; }
	void setGasAlert(bool v) { gasAlert_ = v; }
	void setCompartmentDirty(bool v) { compartmentDirty_ = v; }

	CylinderState state() const { return state_; }
	bool busy() const { return action_.active; }

private:
	// ---- Config & hardware ----
	ConfigStore &cfg_;
	// Pines (ajustar a tu tabla):
	const uint8_t STEP1 = 2, DIR1 = 3, EN1 = 4;
	const uint8_t STEP2 = 5, DIR2 = 6, EN2 = 7;
	const uint8_t STEP3 = 8, DIR3 = 9, EN3 = 10; // motor 3 (solo izquierda, pendiente)

	// Calibración (ajustar según microstepping real):
	const long STEPS_PER_REV = 200 * 8; // 200 pasos * microstepping 1/8 ejemplo

	// Dirección física de cada motor para mapear rotación de cilindro coherente.
	// Si al probar ves que un motor gira invertido, invierte el boolean.
	const bool MOTOR1_DIR_NORMAL = true;  // true => HIGH = derecha cilindro
	const bool MOTOR2_DIR_NORMAL = false; // false => HIGH = izquierda cilindro (inverso)

	unsigned long lastStepMicros_ = 0;
	unsigned int stepDelayMicros_ = 600; // velocidad (ajustable)

	// Estado actual del cilindro
	CylinderState state_ = CYL_SIN_ARENA;
	CylinderAction action_;

	// Seguridad
	bool catPresent_ = false;
	bool catInside_ = false;
	bool gasAlert_ = false;
	bool compartmentDirty_ = false; // demasiado lleno para limpiar

	// Métodos internos
	bool safetyBlocks() const; // true si NO se puede mover
	void startRotationDegrees(int dir, int degrees, CylinderState targetStateAfter, bool multiPhase=false);
	void completeAction();
	void stepBoth(int dir);
	long degreesToSteps(int degrees) const { return (long)(( (double)degrees / 360.0) * (double)STEPS_PER_REV); }
};
