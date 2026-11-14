#include "Telemetry.h"
#include "TelemetryFormat.h"
#include "Clock.h"
#include "Config.h"
#include "Logger.h"
#include "DistanceSensors.h"
#include "Encoder.h"
#include "Gyro.h"
#include "MotorController.h"
#include "RobotController.h"

namespace {
TelemetryData::Snapshot snapshot;
uint32_t lastSnapshotMs = 0;
uint8_t nextPart = TelemetryData::PARTS;
bool requested = false;
static_assert(Config::TELEMETRY_INTERVAL_MS > 0, "Telemetry interval must be positive");
static_assert(Config::SERIAL_DISTANCE_DECIMALS >= 0 && Config::SERIAL_DISTANCE_DECIMALS <= 3 &&
              Config::SERIAL_GYRO_DECIMALS >= 0 && Config::SERIAL_GYRO_DECIMALS <= 3, "Telemetry supports 0..3 decimals");
static_assert(MotorController::STOPPED == 0 && MotorController::FORWARD == 1 &&
              MotorController::BACKWARD == 2 && MotorController::LEFT == 3 && MotorController::RIGHT == 4, "Update motion tags");
static_assert(RobotController::GYRO_FAULT == 0 && RobotController::ENCODER_FAULT == 1 &&
              RobotController::FRONT_FAULT == 2 && RobotController::SIDE_FAULT == 3 &&
              RobotController::TURN_FAULT == 4 && RobotController::NO_FAULT == 5, "Update fault tags");

void capture(uint32_t now) {
    const auto d = DistanceSensors::readings();
    const auto ticks = Encoder::read();
    const auto gyro = Gyro::cachedYawRate();
    const auto state = RobotController::status();
    snapshot.timestamp = now;
    snapshot.front = TelemetryData::reading(d.front, d.front >= 0, DistanceSensors::frontAgeMs(), Safety::FRONT_MAX_AGE_MS);
    snapshot.right = TelemetryData::reading(d.right, d.right >= 0, DistanceSensors::rightAgeMs(), Safety::SIDE_MAX_AGE_MS);
    snapshot.left = TelemetryData::reading(d.left, d.left >= 0, DistanceSensors::leftAgeMs(), Safety::SIDE_MAX_AGE_MS);
    snapshot.gyroZ = TelemetryData::reading(gyro.rate, gyro.valid, gyro.ageMs, Config::TELEMETRY_GYRO_MAX_AGE_MS);
    snapshot.encoderLeft = ticks.left;
    snapshot.encoderRight = ticks.right;
    snapshot.motion = MotorController::motion();
    snapshot.pwmLeft = MotorController::appliedLeftPWM();
    snapshot.pwmRight = MotorController::appliedRightPWM();
    snapshot.fault = state.fault;
    snapshot.automatic = state.automatic;
    snapshot.stopped = state.stopLatched;
    snapshot.pending = state.pendingDrive;
    snapshot.gyroState = Gyro::faulted() ? 2 : (Gyro::available() ? 1 : 0);
}
}

namespace Telemetry {
void begin() {
    lastSnapshotMs = millis();
    nextPart = TelemetryData::PARTS;
    requested = false;
}
void request() { requested = true; }
void update() {
    if (!Config::TELEMETRY_ENABLED) return;
    const uint32_t now = millis();
    if (nextPart < TelemetryData::PARTS &&
        Clock::elapsedMs(now, snapshot.timestamp) > Config::TELEMETRY_FRAME_MAX_AGE_MS) {
        nextPart = TelemetryData::PARTS; // Drop remainder, never replay stale backlog.
    }
    const uint32_t elapsed = Clock::elapsedMs(now, lastSnapshotMs);
    if (nextPart == TelemetryData::PARTS &&
        (elapsed >= Config::TELEMETRY_INTERVAL_MS ||
         (requested && elapsed >= Config::MIN_PRINT_INTERVAL_MS))) {
        capture(now);
        nextPart = 0;
        lastSnapshotMs = now;
        requested = false;
    }
    if (nextPart == TelemetryData::PARTS ||
        Serial.availableForWrite() < TelemetryData::LINE_BYTES - 1) return;
    char line[TelemetryData::LINE_BYTES];
    const size_t length = TelemetryData::format(line, sizeof(line), snapshot, nextPart,
        Config::SERIAL_DISTANCE_DECIMALS, Config::SERIAL_GYRO_DECIMALS);
    if (length == 0 || Logger::tryWrite(line, length)) ++nextPart;
}
}
