#pragma once
namespace Motors {
    void begin();
    void driveForward(int correction = 0);
    void turnRight();
    void turnLeft();
    void stop();
}