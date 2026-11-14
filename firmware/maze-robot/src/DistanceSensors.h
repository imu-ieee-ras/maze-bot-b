
#pragma once

#include <Arduino.h>

namespace DistanceSensors {

struct Distances {
    float front;
    float left;
    float right;
};

void begin();
bool update();
void requestScan();
Distances readings();
unsigned long frontAgeMs();
unsigned long leftAgeMs();
unsigned long rightAgeMs();
bool sidesFresh(unsigned long maxAgeMs);

}

