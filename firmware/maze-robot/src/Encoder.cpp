
#include "Encoder.h"
#include "Config.h"
#include <util/atomic.h>

namespace {

volatile unsigned long leftTicks = 0;
volatile unsigned long rightTicks = 0;

void onLeftEncoder() {
    ++leftTicks;
}

void onRightEncoder() {
    ++rightTicks;
}

}

namespace Encoder {

void begin() {
    pinMode(Pins::LEFT_ENCODER, INPUT_PULLUP);
    pinMode(Pins::RIGHT_ENCODER, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(Pins::LEFT_ENCODER),
        onLeftEncoder,
        RISING
    );

    attachInterrupt(
        digitalPinToInterrupt(Pins::RIGHT_ENCODER),
        onRightEncoder,
        RISING
    );
}

Counts read() {
    Counts result;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        result.left = leftTicks;
        result.right = rightTicks;
    }

    return result;
}

}

