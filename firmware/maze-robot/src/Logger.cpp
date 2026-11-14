#include "Logger.h"
#include "Clock.h"
#include "Config.h"
#include <stdio.h>
#include <avr/pgmspace.h>

namespace {
Clock::LoopClock loopClock;
// Fits the Uno's default 63-byte usable UART TX buffer, including worst-case
// uint32 values. No heap, persistent log queue, Serial.flush() or extra delay.
constexpr size_t CLOCK_LINE_BYTES = 64;
static_assert(Config::CLOCK_LOG_INTERVAL_MS > 0, "Clock log interval must be positive");
}

namespace Logger {
void boot(const __FlashStringHelper *message, unsigned long timestamp) {
    if (!Config::BOOT_LOG_ENABLED) return;
    Serial.print('[');
    Serial.print(timestamp);
    Serial.print(F(" ms] BOOT "));
    Serial.println(message);
}

void begin() {
    loopClock.begin(millis());
}

void update() {
    if (!Config::CLOCK_LOG_ENABLED) return;
    const uint32_t now = millis();
    Clock::Sample sample;
    if (!loopClock.tick(now, Config::CLOCK_LOG_INTERVAL_MS, sample)) return;

    // Drop this sample if busy; never queue it or wait for serial capacity.
    if (Serial.availableForWrite() < static_cast<int>(CLOCK_LINE_BYTES - 1)) return;
    char line[CLOCK_LINE_BYTES];
    const int length = snprintf_P(
        line, sizeof(line), PSTR("[%lu ms] CLOCK up_s=%lu hz=%lu.%03lu\n"),
        static_cast<unsigned long>(now),
        static_cast<unsigned long>(sample.uptimeMs / 1000ULL),
        static_cast<unsigned long>(sample.milliHz / 1000UL),
        static_cast<unsigned long>(sample.milliHz % 1000UL)
    );
    // Only the main thread writes Serial; its ISR only frees TX space.
    if (length > 0 && length < static_cast<int>(sizeof(line)) &&
        Serial.availableForWrite() >= length) {
        Serial.write(reinterpret_cast<const uint8_t *>(line), length);
    }
}
}
