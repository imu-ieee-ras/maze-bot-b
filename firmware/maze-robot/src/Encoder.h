#pragma once

#include <Arduino.h>

namespace Encoder {
struct Counts {
  unsigned long left;
  unsigned long right;
};

void begin();
Counts read();
}
