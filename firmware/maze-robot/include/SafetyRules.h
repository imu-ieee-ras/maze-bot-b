
#pragma once

#include <stdint.h>

namespace SafetyRules {

// S and M are stop commands.
inline bool isStopCommand(char command) {
    return command == 'S' || command == 'M';
}

// A manual command expires after its time limit.
inline bool manualExpired(
    uint32_t now,
    uint32_t lastCommand,
    uint32_t timeoutMs
) {
    return static_cast<uint32_t>(
        now - lastCommand
    ) > timeoutMs;
}

// Invalid, close or stale measurements are unsafe.
inline bool frontClear(
    float distanceCm,
    uint32_t ageMs,
    float minimumCm,
    uint32_t maxAgeMs
) {
    return distanceCm > minimumCm &&
           ageMs <= maxAgeMs;
}

// Detect lack of encoder progress on either wheel.
inline bool encoderStalled(
    uint32_t now,
    uint32_t sessionStart,
    uint32_t lastLeftPulse,
    uint32_t lastRightPulse,
    uint32_t graceMs,
    uint32_t stallMs
) {
    return static_cast<uint32_t>(
        now - sessionStart
    ) >= graceMs &&
    (
        static_cast<uint32_t>(
            now - lastLeftPulse
        ) > stallMs ||
        static_cast<uint32_t>(
            now - lastRightPulse
        ) > stallMs
    );
}

}
