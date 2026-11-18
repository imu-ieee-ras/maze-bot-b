#pragma once
#include <Arduino.h>

namespace Sensors {
    extern bool mpuOk;
    void begin();
    float getDistance(uint8_t trigPin, uint8_t echoPin);
    float readGyroZ();
    void resetHeading();
    void updateHeading();
    float getHeading();
}