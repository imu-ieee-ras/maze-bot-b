
#include "Gyro.h"
#include "Config.h"
#include "Logger.h"
#include <Wire.h>

namespace {
bool ready = false;
bool runtimeFault = false;
float biasZ = 0.0f;

bool writeReg(uint8_t address, uint8_t value) {
    Wire.clearWireTimeoutFlag();

    Wire.beginTransmission(Config::MPU_ADDRESS);
    Wire.write(address);
    Wire.write(value);

    return Wire.endTransmission() == 0 &&
           !Wire.getWireTimeoutFlag();
}

bool readBytes(uint8_t address, uint8_t *out, uint8_t count) {
    Wire.clearWireTimeoutFlag();

    Wire.beginTransmission(Config::MPU_ADDRESS);
    Wire.write(address);

    if (Wire.endTransmission(false) != 0 ||
        Wire.getWireTimeoutFlag()) {
        return false;
    }

    if (Wire.requestFrom(Config::MPU_ADDRESS, count) != count ||
        Wire.getWireTimeoutFlag()) {
        return false;
    }

    for (uint8_t i = 0; i < count; ++i) {
        out[i] = Wire.read();
    }

    return true;
}

bool readRawZ(int16_t &raw) {
    if (!ready || runtimeFault) return false;

    uint8_t bytes[2];

    if (!readBytes(0x47, bytes, 2)) {
        runtimeFault = true;
        ready = false;
        return false;
    }

    raw = static_cast<int16_t>(
        (static_cast<uint16_t>(bytes[0]) << 8) |
        bytes[1]
    );

    return true;
}
}

namespace Gyro {

void begin() {
    Wire.begin();

    Wire.setWireTimeout(
        Safety::WIRE_TIMEOUT_US, true
    );

    ready = false;
    runtimeFault = false;

    uint8_t who[1];

    if (!readBytes(0x75, who, 1) ||
        who[0] != 0x68 ||
        !writeReg(0x6B, 0x00) ||
        !writeReg(0x1B, 0x00)) {

        Logger::boot(F(
            "WARN: MPU6050 missing/config failed; AUTO disabled"
        ));
        return;
    }

    ready = true;
    delay(Config::GYRO_STARTUP_DELAY_MS);

    Logger::boot(F(
        "MPU6050: bias calibration; keep robot STILL"
    ));

    long sum = 0;
    uint16_t ok = 0;

    for (uint16_t i = 0; i < Config::GYRO_CALIBRATION_SAMPLES; ++i) {
        int16_t z;

        if (readRawZ(z)) {
            sum += z;
            ++ok;
        }

        if (runtimeFault) break;

        delay(Config::GYRO_CALIBRATION_SAMPLE_DELAY_MS);
    }

    if (ok < Config::GYRO_CALIBRATION_MIN_SAMPLES || runtimeFault) {
        ready = false;

        Logger::boot(F(
            "WARN: MPU6050 calibration failed"
        ));
        return;
    }

    biasZ = static_cast<float>(sum) / ok;

    Logger::boot(F("MPU6050: I2C identity/read verified; bias computed"));
}

bool available() {
    return ready && !runtimeFault;
}

bool faulted() {
    return runtimeFault;
}

bool yawRateDps(float &value) {
    int16_t z;

    if (!readRawZ(z)) return false;

    value = (static_cast<float>(z) - biasZ) / 131.0f;

    return true;
}

}

