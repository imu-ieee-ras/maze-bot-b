#include "Communication.h"
#include "Config.h"
#include "Telemetry.h"

namespace Communication {
void begin() { Serial.begin(Config::SERIAL_BAUD); }

bool nextCommand(char &command) {
  if (!Serial.available()) return false;
  command = static_cast<char>(Serial.read());
  if (command >= 'a' && command <= 'z') command -= 'a' - 'A';
  return true;
}

bool stopRequestedDuringTurn() {
  char command;
  while (nextCommand(command)) {
    // Ignore all but an emergency stop while a turn is in progress.
    if (command == 'S' || command == 'M') return true;
  }
  return false;
}

void printReadings(bool autoMode) {
  (void)autoMode; // Controller status is captured when the sample is taken.
  Telemetry::request();
}
}
