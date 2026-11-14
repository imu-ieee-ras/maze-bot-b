
#pragma once

#include <Arduino.h>

namespace Pins {
constexpr uint8_t LEFT_EN = 9;
constexpr uint8_t LEFT_IN1 = 4;
constexpr uint8_t LEFT_IN2 = 5;

constexpr uint8_t RIGHT_IN3 = 6;
constexpr uint8_t RIGHT_IN4 = 7;
constexpr uint8_t RIGHT_EN = 10;

constexpr uint8_t LEFT_ENCODER = 2;
constexpr uint8_t RIGHT_ENCODER = 3;

constexpr uint8_t FRONT_TRIG = A0;
constexpr uint8_t FRONT_ECHO = A1;

constexpr uint8_t LEFT_TRIG = A2;
constexpr uint8_t LEFT_ECHO = A3;

constexpr uint8_t RIGHT_TRIG = 13;
constexpr uint8_t RIGHT_ECHO = 8;

// MPU6050: SDA=A4, SCL=A5
}

namespace Config {
// Boot messages are setup-only; clock output is dropped when UART TX is busy.
constexpr bool BOOT_LOG_ENABLED = true;
constexpr bool CLOCK_LOG_ENABLED = true;
constexpr unsigned long CLOCK_LOG_INTERVAL_MS = 5000UL;

// USB serial output: timing and decimal precision (existing text format).
constexpr uint32_t SERIAL_BAUD = 115200UL;
constexpr unsigned long PRINT_INTERVAL_MS = 750UL;
constexpr unsigned long MIN_PRINT_INTERVAL_MS = 500UL;
constexpr int SERIAL_DISTANCE_DECIMALS = 1;
constexpr int SERIAL_GYRO_DECIMALS = 2;

// Bounded RX draining for stop/re-arm; does not resize the UART buffer.
constexpr int SERIAL_DRAIN_LIMIT = 64;

// Sensor address and startup calibration; keep the robot stationary.
constexpr uint8_t MPU_ADDRESS = 0x68;
constexpr int GYRO_STARTUP_DELAY_MS = 100;
constexpr int GYRO_CALIBRATION_SAMPLES = 200;
constexpr int GYRO_CALIBRATION_SAMPLE_DELAY_MS = 5;
constexpr int GYRO_CALIBRATION_MIN_SAMPLES = 150;

// Motor polarity, drive levels and stopping/settling times.
constexpr bool REVERSE_LEFT_MOTOR = false;
constexpr bool REVERSE_RIGHT_MOTOR = false;

constexpr int MAX_DRIVE_PWM = 160;
constexpr int CRUISE_PWM = 140;
constexpr int SLOW_PWM = 95;
constexpr int TURN_PWM = 120;
constexpr int BACKWARD_PWM = 120;
constexpr int BRAKE_DURATION_MS = 65;
constexpr unsigned long DIRECTION_CHANGE_DELAY_MS = 100UL;
constexpr int TURN_SETTLE_MS = 100;

// Control: P correction per encoder tick/second difference.
constexpr float BALANCE_KP = 0.6f;
constexpr int MAX_BALANCE_CORRECTION = 30;
constexpr unsigned long BALANCE_INTERVAL_MS = 250UL;
constexpr float TURN_STOP_MARGIN_DEG = 5.0f;
constexpr int TURN_POLL_DELAY_MS = 5;

// Conservative initial thresholds; hardware calibration required
constexpr float FRONT_BRAKE_CM = 20.0f;
constexpr float FRONT_SLOW_CM = 40.0f;
constexpr float SIDE_OPEN_CM = 20.0f;

// Sonar conversion and accepted range, followed by acquisition timing.
constexpr float SONAR_US_PER_CM = 58.0f;
constexpr float SONAR_MIN_CM = 2.0f;
constexpr float SONAR_MAX_CM = 400.0f;
constexpr unsigned long SONAR_TIMEOUT_US = 30000UL;
constexpr unsigned long INTER_SONAR_DELAY_MS = 60UL;
}

namespace Safety {
// Communication and manual-control time limits.
constexpr unsigned long WIRE_TIMEOUT_US = 25000UL;
constexpr unsigned long MANUAL_LEASE_MS = 1500UL;

// Encoder progress and sensor freshness limits.
constexpr unsigned long ENCODER_GRACE_MS = 700UL;
constexpr unsigned long ENCODER_STALL_MS = 1200UL;
constexpr unsigned long FRONT_MAX_AGE_MS = 450UL;
constexpr unsigned long SIDE_MAX_AGE_MS = 750UL;
constexpr unsigned long GYRO_CHECK_MS = 250UL;

// Turn watchdogs and maximum tolerated rotation in the wrong direction.
constexpr int TURN_LONG_THRESHOLD_DEG = 120;
constexpr unsigned long TURN_LONG_TIMEOUT_MS = 4500UL;
constexpr unsigned long TURN_SHORT_TIMEOUT_MS = 2700UL;
constexpr float TURN_REVERSE_LIMIT_DEG = 12.0f;

// Bound command processing so safety supervision can keep running.
constexpr uint8_t MAX_COMMANDS_PER_LOOP = 16;
}

