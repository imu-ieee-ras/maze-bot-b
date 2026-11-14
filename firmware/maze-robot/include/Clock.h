#pragma once

#include <stdint.h>

namespace Clock {
inline uint32_t elapsedMs(uint32_t now, uint32_t since) { return now - since; }

struct Sample {
    uint64_t uptimeMs;
    uint32_t windowMs;
    uint32_t loops;
    uint32_t milliHz;
};

// Call once per completed main loop, at least once per millis() wrap.
// Measures wall time, including blocking sensor reads and turns.
class LoopClock {
public:
    void begin(uint32_t now) {
        lastMs_ = windowStartMs_ = now;
        uptimeMs_ = now; // Include setup time in uptime.
        loops_ = 0;
    }

    bool tick(uint32_t now, uint32_t intervalMs, Sample &sample) {
        uptimeMs_ += static_cast<uint32_t>(now - lastMs_);
        lastMs_ = now;
        ++loops_;
        const uint32_t elapsed = now - windowStartMs_;
        if (intervalMs == 0 || elapsed == 0 || elapsed < intervalMs) {
            return false;
        }

        sample.uptimeMs = uptimeMs_;
        sample.windowMs = elapsed;
        sample.loops = loops_;
        sample.milliHz = static_cast<uint32_t>(
            static_cast<uint64_t>(loops_) * 1000000ULL / elapsed
        );
        windowStartMs_ = now;
        loops_ = 0;
        return true;
    }

private:
    uint64_t uptimeMs_ = 0;
    uint32_t lastMs_ = 0;
    uint32_t windowStartMs_ = 0;
    uint32_t loops_ = 0;
};
}
