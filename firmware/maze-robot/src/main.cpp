#include <Arduino.h>
#include "RobotController.h"
#include "Logger.h"
#include "Telemetry.h"

void setup() {
  RobotController::begin();
  Logger::begin();
  Telemetry::begin();
}

void loop() {
  RobotController::update();
  Logger::update();
  Telemetry::update();
}
