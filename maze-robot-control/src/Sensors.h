#pragma once
#include <Arduino.h>

namespace Sensors {
    extern bool mpuOk;
    extern uint8_t mpuAddr;
    void begin();
    float getDistance(uint8_t trigPin, uint8_t echoPin);
    void resetDistances();
    float readGyroZ();
    void resetHeading();
    void updateHeading();
    float getHeading();
}
