#pragma once
namespace Telemetry {
void begin();
void request(); // Coalesced P command; existing minimum print interval applies.
void update(); // Cached data only; sends at most one complete line per call.
}
