#pragma once

#include <Arduino.h>

namespace Logger {
// Boot-only synchronous output: call after motor outputs have been stopped.
void boot(const __FlashStringHelper *message, unsigned long timestamp = millis());
// Start loop measurement after setup. Uptime still includes setup time.
void begin();
// Call after RobotController::update(), including its early-return paths.
void update();
// Shared non-blocking whole-line output for Clock and Telemetry.
bool tryWrite(const char *line, size_t length);
}
