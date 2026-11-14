#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#ifdef __AVR__
#include <avr/pgmspace.h>
#define TEL_TEXT(s) PSTR(s)
#define TEL_PRINTF snprintf_P
#define TEL_TAG "%S"
#else
#define TEL_TEXT(s) (s)
#define TEL_PRINTF snprintf
#define TEL_TAG "%s"
#endif

namespace TelemetryData {
constexpr uint8_t LINE_BYTES = 64;
constexpr uint8_t PARTS = 5;
enum Quality : uint8_t { VALID, INVALID, STALE };
struct Reading { float value; Quality quality; };
struct Snapshot {
    uint32_t timestamp;
    Reading front, right, left, gyroZ;
    uint32_t encoderLeft, encoderRight;
    uint8_t motion, pwmLeft, pwmRight, fault, gyroState;
    bool automatic, stopped, pending;
};

inline Reading reading(float value, bool valid, uint32_t age, uint32_t maxAge) {
    if (!valid || !isfinite(value) || fabsf(value) > 1000000.0f) return {0, INVALID};
    return {value, age > maxAge ? STALE : VALID};
}

inline void valueText(char *out, size_t capacity, Reading value, uint8_t decimals) {
    if (value.quality != VALID || !isfinite(value.value) ||
        fabsf(value.value) > 1000000.0f || decimals > 3) {
        TEL_PRINTF(out, capacity, value.quality == STALE ? TEL_TEXT("STALE") : TEL_TEXT("INV"));
        return;
    }
    uint32_t scale = 1;
    for (uint8_t i = 0; i < decimals; ++i) scale *= 10;
    const uint32_t rounded = static_cast<uint32_t>(fabsf(value.value) * scale + 0.5f);
    const bool negative = value.value < 0 && rounded != 0;
    if (decimals == 0) {
        TEL_PRINTF(out, capacity, TEL_TEXT("%s%lu"), negative ? "-" : "", static_cast<unsigned long>(rounded));
    } else {
        // AVR libc does not support '*' width/precision in printf.
        const char *fmt = decimals == 1 ? TEL_TEXT("%s%lu.%01lu") :
            (decimals == 2 ? TEL_TEXT("%s%lu.%02lu") : TEL_TEXT("%s%lu.%03lu"));
        TEL_PRINTF(out, capacity, fmt, negative ? "-" : "",
            static_cast<unsigned long>(rounded / scale),
            static_cast<unsigned long>(rounded % scale));
    }
}

// Codes match the explicitly checked controller enums in Telemetry.cpp.
inline const char *motionTag(uint8_t motion) {
    switch (motion) {
        case 0: return TEL_TEXT("STOPPED");
        case 1: return TEL_TEXT("FORWARD");
        case 2: return TEL_TEXT("BACKWARD");
        case 3: return TEL_TEXT("LEFT");
        case 4: return TEL_TEXT("RIGHT");
        default: return TEL_TEXT("UNK");
    }
}
inline const char *faultTag(uint8_t fault) {
    switch (fault) {
        case 0: return TEL_TEXT("GYRO");
        case 1: return TEL_TEXT("ENCODER");
        case 2: return TEL_TEXT("FRONT");
        case 3: return TEL_TEXT("SIDE");
        case 4: return TEL_TEXT("TURN");
        case 5: return TEL_TEXT("NONE");
        default: return TEL_TEXT("UNK");
    }
}

inline size_t format(char *out, size_t capacity, const Snapshot &s, uint8_t part,
                     uint8_t distanceDecimals, uint8_t gyroDecimals) {
    if (capacity == 0) return 0;
    const unsigned long time = s.timestamp;
    int length = -1;
    if (part == 0) {
        char front[16], right[16], left[16];
        valueText(front, sizeof(front), s.front, distanceDecimals);
        valueText(right, sizeof(right), s.right, distanceDecimals);
        valueText(left, sizeof(left), s.left, distanceDecimals);
        length = TEL_PRINTF(out, capacity, TEL_TEXT("[%lu ms] TEL D cm F=%s R=%s L=%s\n"), time, front, right, left);
    } else if (part == 1) {
        length = TEL_PRINTF(out, capacity, TEL_TEXT("[%lu ms] TEL E L=%lu R=%lu\n"), time,
            static_cast<unsigned long>(s.encoderLeft), static_cast<unsigned long>(s.encoderRight));
    } else if (part == 2) {
        char rate[16];
        valueText(rate, sizeof(rate), s.gyroZ, gyroDecimals);
        length = TEL_PRINTF(out, capacity, TEL_TEXT("[%lu ms] TEL G z_dps=%s\n"), time, rate);
    } else if (part == 3) {
        length = TEL_PRINTF(out, capacity, TEL_TEXT("[%lu ms] TEL M " TEL_TAG " pwm=%u,%u " TEL_TAG "\n"), time,
            motionTag(s.motion), static_cast<unsigned int>(s.pwmLeft), static_cast<unsigned int>(s.pwmRight),
            s.automatic ? TEL_TEXT("AUTO") : TEL_TEXT("MANUAL"));
    } else if (part == 4) {
        const char *gyro = s.gyroState == 2 ? TEL_TEXT("FAULT") :
            (s.gyroState == 1 ? TEL_TEXT("READY") : TEL_TEXT("MISSING"));
        length = TEL_PRINTF(out, capacity, TEL_TEXT("[%lu ms] TEL S fault=" TEL_TAG " stop=%u pend=%u gyro=" TEL_TAG "\n"), time,
            faultTag(s.fault), static_cast<unsigned int>(s.stopped), static_cast<unsigned int>(s.pending), gyro);
    }
    if (length <= 0 || static_cast<size_t>(length) >= capacity || length >= LINE_BYTES) {
        out[0] = '\0'; // Never emit a truncated/malformed record.
        return 0;
    }
    return static_cast<size_t>(length);
}
}
#undef TEL_TEXT
#undef TEL_PRINTF
#undef TEL_TAG
