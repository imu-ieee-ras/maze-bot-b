#include <Arduino.h>
#include "RobotController.h"
#include "Logger.h"

void setup() {
  RobotController::begin();
  Logger::begin();
}

void loop() {
  RobotController::update();
  Logger::update();
}
