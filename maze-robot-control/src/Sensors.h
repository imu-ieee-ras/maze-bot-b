#pragma once
#include <Arduino.h>

namespace Sensors {
    void begin();
    float getDistance(uint8_t trigPin, uint8_t echoPin);
    float getYawAngle(); 
    void resetYawAngle();
    unsigned long getLeftTicks();
    unsigned long getRightTicks();
    void resetTicks();
}