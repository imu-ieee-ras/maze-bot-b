
#pragma once

#include <Arduino.h>

namespace Gyro {
void begin();
bool available();
bool faulted();
bool yawRateDps(float &value);
struct CachedRate { float rate; unsigned long ageMs; bool valid; };
CachedRate cachedYawRate(); // Does not perform an I2C transaction.
}

