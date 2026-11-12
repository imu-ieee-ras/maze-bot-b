
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
constexpr uint32_t SERIAL_BAUD = 115200UL;
constexpr uint8_t MPU_ADDRESS = 0x68;

constexpr bool REVERSE_LEFT_MOTOR = false;
constexpr bool REVERSE_RIGHT_MOTOR = false;

constexpr int MAX_DRIVE_PWM = 160;
constexpr int CRUISE_PWM = 140;
constexpr int SLOW_PWM = 95;
constexpr int TURN_PWM = 120;
constexpr int BACKWARD_PWM = 120;

// P control: correction per encoder tick/second difference
constexpr float BALANCE_KP = 0.6f;
constexpr int MAX_BALANCE_CORRECTION = 30;

// Conservative initial thresholds; hardware calibration required
constexpr float FRONT_BRAKE_CM = 20.0f;
constexpr float FRONT_SLOW_CM = 40.0f;
constexpr float SIDE_OPEN_CM = 20.0f;

constexpr unsigned long SONAR_TIMEOUT_US = 30000UL;
constexpr unsigned long INTER_SONAR_DELAY_MS = 60UL;
constexpr unsigned long BALANCE_INTERVAL_MS = 250UL;
constexpr unsigned long PRINT_INTERVAL_MS = 750UL;
}

namespace Safety {
constexpr unsigned long WIRE_TIMEOUT_US = 25000UL;
constexpr unsigned long MANUAL_LEASE_MS = 1500UL;
constexpr unsigned long ENCODER_GRACE_MS = 700UL;
constexpr unsigned long ENCODER_STALL_MS = 1200UL;
constexpr unsigned long FRONT_MAX_AGE_MS = 450UL;
constexpr unsigned long SIDE_MAX_AGE_MS = 750UL;
constexpr unsigned long GYRO_CHECK_MS = 250UL;
constexpr uint8_t MAX_COMMANDS_PER_LOOP = 16;
}

