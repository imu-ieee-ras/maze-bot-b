#pragma once

#include <Arduino.h>

// USB Serial is the only communication interface.
namespace Communication {
void begin();
bool nextCommand(char &command);
bool stopRequestedDuringTurn();
void printReadings(bool autoMode);
}
