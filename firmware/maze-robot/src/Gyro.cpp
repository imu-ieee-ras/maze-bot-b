
#include "Gyro.h"
#include "Config.h"
#include "Logger.h"
#include <Wire.h>

namespace {
// MPU-6050 RM-MPU-6000A-00 and MPU-6500 RM-MPU-6500A-00:
// the registers used here are shared; WHO_AM_I is NOT the I2C address.
constexpr uint8_t WHO_AM_I = 0x75;
constexpr uint8_t MPU6050_ID = 0x68;
constexpr uint8_t MPU6500_ID = 0x70;
constexpr uint8_t PWR_MGMT_1 = 0x6B;
constexpr uint8_t PWR_MGMT_2 = 0x6C;
constexpr uint8_t GYRO_CONFIG = 0x1B;
constexpr uint8_t FILTER_CONFIG = 0x1A;
constexpr uint8_t GYRO_ZOUT_H = 0x47;
constexpr float GYRO_LSB_PER_DPS = 131.0f; // FS_SEL=0: +/-250 degrees/s.

// MPU-6500 PS-MPU-6500A-01 allows up to 100 ms before register access.
// Also retain at least 100 ms after reset and before bias calibration.
static_assert(Config::GYRO_STARTUP_DELAY_MS >= 100,
              "MPU startup/reset settling requires at least 100 ms");
bool ready = false;
bool runtimeFault = false;
float biasZ = 0.0f;
float lastRate = 0.0f;
unsigned long lastRateMs = 0;
bool haveRate = false;

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

    if (!readBytes(GYRO_ZOUT_H, bytes, 2)) {
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
    haveRate = false;

    // Wait BEFORE the first identity read, not only before calibration.
    delay(Config::GYRO_STARTUP_DELAY_MS);
    uint8_t who;
    if (!readBytes(WHO_AM_I, &who, 1)) {
        Logger::boot(F("WARN: gyro identity read failed; AUTO disabled"));
        return;
    }
    if (who == MPU6050_ID) {
        Logger::boot(F("Gyro: detected MPU6050 (WHO_AM_I=0x68)"));
    } else if (who == MPU6500_ID) {
        Logger::boot(F("Gyro: detected MPU6500 (WHO_AM_I=0x70)"));
    } else {
        Logger::boot(F("WARN: unsupported gyro identity; AUTO disabled"));
        return;
    }

    // Reset only an identified device; remove settings left by earlier firmware.
    if (!writeReg(PWR_MGMT_1, 0x80)) {
        Logger::boot(F("WARN: gyro reset failed; AUTO disabled"));
        return;
    }
    delay(Config::GYRO_STARTUP_DELAY_MS);

    // CLKSEL=1: X-gyro PLL on MPU6050; automatic PLL selection on MPU6500.
    // Wake all axes. FS_SEL=0 gives the same 131 LSB/(degrees/s) on both.
    // GYRO_CONFIG=0 also clears MPU6500 FCHOICE_B (reserved on MPU6050).
    if (!writeReg(PWR_MGMT_1, 0x01) ||
        !writeReg(PWR_MGMT_2, 0x00) ||
        !writeReg(FILTER_CONFIG, 0x00) ||
        !writeReg(GYRO_CONFIG, 0x00)) {
        Logger::boot(F("WARN: gyro configuration failed; AUTO disabled"));
        return;
    }
    delay(Config::GYRO_STARTUP_DELAY_MS);

    // ACK alone does not prove that the scale/power settings took effect.
    uint8_t identity, power[2], config[2];
    if (!readBytes(WHO_AM_I, &identity, 1) || identity != who ||
        !readBytes(PWR_MGMT_1, power, 2) ||
        power[0] != 0x01 || power[1] != 0x00 ||
        !readBytes(FILTER_CONFIG, config, 2) ||
        config[0] != 0x00 || config[1] != 0x00) {
        Logger::boot(F("WARN: gyro configuration verification failed; AUTO disabled"));
        return;
    }

    ready = true;
    Logger::boot(F(
        "Gyro: bias calibration; keep robot STILL"
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
            "WARN: gyro calibration failed"
        ));
        return;
    }

    biasZ = static_cast<float>(sum) / ok;

    Logger::boot(F("Gyro: I2C identity/config/read verified; bias computed"));
}

bool available() {
    return ready && !runtimeFault;
}

bool faulted() {
    return runtimeFault;
}

CachedRate cachedYawRate() {
    return {lastRate, haveRate ? millis() - lastRateMs : ~0UL, haveRate && available()};
}

bool yawRateDps(float &value) {
    int16_t z;

    if (!readRawZ(z)) return false;

    value = (static_cast<float>(z) - biasZ) / GYRO_LSB_PER_DPS;
    lastRate = value;
    lastRateMs = millis();
    haveRate = true;

    return true;
}

}

