#pragma once
namespace Motors {
    void begin();
    void drive(int leftSpeed, int rightSpeed);
    void activeBrake();
    void turnDegrees(float degrees, bool toRight);
}