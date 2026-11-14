#pragma once
#include <stddef.h>
#include <stdint.h>

namespace LogOutput {
// Single main-thread writer; UART ISR may only free space between these calls.
// No partial writes: callers can retry/drop whole lines without interleaving.
template <typename Sink>
bool tryWrite(Sink &sink, const char *line, size_t length) {
    const int available = sink.availableForWrite();
    if (length == 0 || available < 0 || static_cast<size_t>(available) < length) {
        return false;
    }
    return sink.write(reinterpret_cast<const uint8_t *>(line), length) == length;
}
}
