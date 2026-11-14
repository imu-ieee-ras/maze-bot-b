#pragma once

#include <stdint.h>

namespace RobotController {
enum Fault : uint8_t { GYRO_FAULT, ENCODER_FAULT, FRONT_FAULT, SIDE_FAULT, TURN_FAULT, NO_FAULT };
struct Status { bool automatic; bool stopLatched; bool pendingDrive; Fault fault; };
Status status();
void begin();
void update();
}
