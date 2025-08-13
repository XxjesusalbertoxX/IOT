#include <Arduino.h>
<<<<<<< Updated upstream

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
=======
#include "sensors/SensorManager.h"

SensorManager sensorManager;

unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 3000;

void setup() {
    Serial.begin(115200);
    while(!Serial) { delay(10); }
    sensorManager.begin();
    Serial.println(F("{\"event\":\"READY\",\"message\":\"All devices initialized\"}"));
}

void loop() {
    sensorManager.poll();

    unsigned long now = millis();
    if (now - lastPrintTime >= PRINT_INTERVAL) {
        Serial.print("{");
        
        // COMEDERO
        FeederWeightSensor* fw = sensorManager.getFeederWeight();
        if (fw && fw->isReady()) {
            Serial.print("\"feeder_weight\":" + String(fw->getCurrentWeight(), 2) + ",");
        }
        
        FeederUltrasonicSensor* fu1 = sensorManager.getFeederUltrasonic1();
        FeederUltrasonicSensor* fu2 = sensorManager.getFeederUltrasonic2();
        if (fu1 && fu1->isReady()) {
            Serial.print("\"feeder_level_1\":" + String(fu1->getDistance(), 1) + ",");
        }
        if (fu2 && fu2->isReady()) {
            Serial.print("\"feeder_level_2\":" + String(fu2->getDistance(), 1) + ",");
        }
        
        // BEBEDERO
        WaterDispenserSensor* wd = sensorManager.getWaterDispenser();
        if (wd && wd->isReady()) {
            Serial.print("\"water_level\":" + String(wd->getAnalogValue(), 0) + ",");
        }
        
        // ARENERO
        LitterboxDHTSensor* ldht = sensorManager.getLitterboxDHT();
        if (ldht && ldht->isReady()) {
            Serial.print("\"temperature\":" + String(ldht->getTemperature(), 1) + ",");
            Serial.print("\"humidity\":" + String(ldht->getHumidity(), 1) + ",");
        }
        
        LitterboxUltrasonicSensor* lu = sensorManager.getLitterboxUltrasonic();
        if (lu && lu->isReady()) {
            Serial.print("\"litterbox_distance\":" + String(lu->getDistance(), 1) + ",");
        }
        
        LitterboxMQ2Sensor* lmq2 = sensorManager.getLitterboxMQ2();
        if (lmq2 && lmq2->isReady()) {
            Serial.print("\"gas_level\":" + String(lmq2->getValue(), 0));
        }
        
        Serial.println("}");
        lastPrintTime = now;
    }
    delay(10);
>>>>>>> Stashed changes
}