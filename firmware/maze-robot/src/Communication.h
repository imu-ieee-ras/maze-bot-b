#pragma once

#include <Arduino.h>

// USB Serial is the only communication interface.
namespace Communication {
void begin();
bool nextCommand(char &command);
bool stopRequestedDuringTurn();
// Compatibility request: output is owned by Telemetry (no sensor reads here).
void printReadings(bool autoMode);
}
