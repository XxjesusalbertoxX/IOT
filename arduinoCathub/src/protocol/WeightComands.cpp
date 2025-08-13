// #include "WeightCommands.h"

// WeightCommands::WeightCommands(WeightSensor* sensor) : weightSensor(sensor) {}

// void WeightCommands::processCommand(String command) {
//     command.trim();
//     command.toUpperCase();
    
//     if (command == "GET_WEIGHT") {
//         float weight = weightSensor->getCurrentWeight();
//         Serial.println("{\"sensor\":\"weight\",\"value\":" + String(weight, 2) + ",\"unit\":\"g\",\"status\":\"" + weightSensor->getStatus() + "\"}");
//     }
//     else if (command == "TARE") {
//         weightSensor->tare();
//         Serial.println("{\"sensor\":\"weight\",\"action\":\"tare\",\"status\":\"OK\"}");
//     }
//     else if (command.startsWith("CALIBRATE:")) {
//         float knownWeight = command.substring(10).toFloat();
//         if (knownWeight > 0) {
//             weightSensor->calibrate(knownWeight);
//             Serial.println("{\"sensor\":\"weight\",\"action\":\"calibrate\",\"weight\":" + String(knownWeight) + ",\"status\":\"OK\"}");
//         } else {
//             Serial.println("{\"sensor\":\"weight\",\"action\":\"calibrate\",\"status\":\"ERROR\",\"message\":\"Invalid weight\"}");
//         }
//     }
//     else if (command == "WEIGHT_STATUS") {
//         Serial.println("{\"sensor\":\"weight\",\"ready\":" + String(weightSensor->isReady() ? "true" : "false") + ",\"status\":\"" + weightSensor->getStatus() + "\"}");
//     }
//     else if (command == "WEIGHT_HELP") {
//         showHelp();
//     }
// }

// void WeightCommands::showHelp() {
//     Serial.println("{\"help\":\"weight_commands\",\"commands\":[");
//     Serial.println("  \"GET_WEIGHT - Get current weight reading\",");
//     Serial.println("  \"TARE - Reset scale to zero\",");
//     Serial.println("  \"CALIBRATE:XXX - Calibrate with known weight XXX grams\",");
//     Serial.println("  \"WEIGHT_STATUS - Get sensor status\",");
//     Serial.println("  \"WEIGHT_HELP - Show this help\"");
//     Serial.println("]}");
// }