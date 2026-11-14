
#include "DistanceSensors.h"
#include "Config.h"

namespace {

DistanceSensors::Distances values = {-1.0f, -1.0f, -1.0f};

uint8_t nextSensor = 0;

unsigned long lastMeasureMs = 0;
unsigned long lastFrontMs = 0;
unsigned long lastLeftMs = 0;
unsigned long lastRightMs = 0;

bool haveFront = false;
bool haveLeft = false;
bool haveRight = false;

float readCm(uint8_t trig, uint8_t echo) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    const unsigned long duration = pulseIn(
        echo, HIGH, Config::SONAR_TIMEOUT_US
    );

    if (duration == 0) return -1.0f;

    const float cm = duration / Config::SONAR_US_PER_CM;

    return (cm >= Config::SONAR_MIN_CM && cm <= Config::SONAR_MAX_CM)
        ? cm : -1.0f;
}

}

namespace DistanceSensors {

void begin() {
    pinMode(Pins::FRONT_TRIG, OUTPUT);
    pinMode(Pins::FRONT_ECHO, INPUT);

    pinMode(Pins::LEFT_TRIG, OUTPUT);
    pinMode(Pins::LEFT_ECHO, INPUT);

    pinMode(Pins::RIGHT_TRIG, OUTPUT);
    pinMode(Pins::RIGHT_ECHO, INPUT);

    digitalWrite(Pins::FRONT_TRIG, LOW);
    digitalWrite(Pins::LEFT_TRIG, LOW);
    digitalWrite(Pins::RIGHT_TRIG, LOW);

    lastMeasureMs =
        millis() - Config::INTER_SONAR_DELAY_MS;
}

bool update() {
    if (millis() - lastMeasureMs <
        Config::INTER_SONAR_DELAY_MS) {
        return false;
    }

    const uint8_t s = nextSensor;

    if (s == 0) {
        values.front = readCm(
            Pins::FRONT_TRIG, Pins::FRONT_ECHO
        );

        lastFrontMs = millis();
        haveFront = true;

    } else if (s == 1) {
        values.left = readCm(
            Pins::LEFT_TRIG, Pins::LEFT_ECHO
        );

        lastLeftMs = millis();
        haveLeft = true;

    } else {
        values.right = readCm(
            Pins::RIGHT_TRIG, Pins::RIGHT_ECHO
        );

        lastRightMs = millis();
        haveRight = true;
    }

    lastMeasureMs = millis();
    nextSensor = (s + 1) % 3;

    return s == 0;
}

void requestScan() {
    nextSensor = 0;
}

Distances readings() {
    return values;
}

unsigned long frontAgeMs() {
    return haveFront
        ? millis() - lastFrontMs
        : ~0UL;
}

bool sidesFresh(unsigned long maxAgeMs) {
    return haveLeft && haveRight &&
           millis() - lastLeftMs <= maxAgeMs &&
           millis() - lastRightMs <= maxAgeMs;
}

}

