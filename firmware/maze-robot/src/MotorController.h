#pragma once

#include <Arduino.h>

namespace MotorController {
enum Motion : uint8_t { STOPPED, FORWARD, BACKWARD, LEFT, RIGHT };

void begin();
void setSpeeds(int leftPWM, int rightPWM);
void stop();
void brake();
void forward(int pwm);
void backward();
void turnLeft();
void turnRight();
Motion motion();
int forwardPWM();
uint8_t appliedLeftPWM();
uint8_t appliedRightPWM();
}
