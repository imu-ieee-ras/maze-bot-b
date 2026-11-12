
#pragma once

#include <Arduino.h>

namespace Gyro {
void begin();
bool available();
bool faulted();
bool yawRateDps(float &value);
}

