#pragma once

namespace Motors {
    void begin();
    void drive(int leftSpeed, int rightSpeed);
    void setDirect(int leftSpeed, int rightSpeed);
    void activeBrake();
}
