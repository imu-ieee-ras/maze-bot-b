#include "Communication.h"
#include "Config.h"
#include "DistanceSensors.h"
#include "Encoder.h"
#include "Gyro.h"

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
  const DistanceSensors::Distances d = DistanceSensors::readings();
  const Encoder::Counts ticks = Encoder::read();
  Serial.print(F("front_cm=")); Serial.print(d.front, Config::SERIAL_DISTANCE_DECIMALS);
  Serial.print(F(" left_cm=")); Serial.print(d.left, Config::SERIAL_DISTANCE_DECIMALS);
  Serial.print(F(" right_cm=")); Serial.print(d.right, Config::SERIAL_DISTANCE_DECIMALS);
  Serial.print(F(" encoder_left=")); Serial.print(ticks.left);
  Serial.print(F(" encoder_right=")); Serial.print(ticks.right);
  Serial.print(F(" gyro_z_dps="));
  float rate;
  if (Gyro::yawRateDps(rate)) Serial.print(rate, Config::SERIAL_GYRO_DECIMALS);
  else Serial.print(F("NA"));
  Serial.print(F(" mode=")); Serial.println(autoMode ? F("AUTO") : F("MANUAL"));
}
}
