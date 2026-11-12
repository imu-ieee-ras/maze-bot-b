#include <Arduino.h>
#include "RobotController.h"

void setup() {
  RobotController::begin();
}

void loop() {
  RobotController::update();
}
