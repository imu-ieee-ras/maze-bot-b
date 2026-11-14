#include <unity.h>
#include <string.h>
#include <math.h>
#include "TelemetryFormat.h"
#include "LogOutput.h"
#include "Clock.h"

void setUp() {}
void tearDown() {}

void test_invalid_and_stale_readings() {
    using namespace TelemetryData;
    TEST_ASSERT_EQUAL(VALID, reading(0, true, 500, 500).quality);
    TEST_ASSERT_EQUAL(STALE, reading(12, true, 501, 500).quality);
    TEST_ASSERT_EQUAL(INVALID, reading(-1, false, 0, 500).quality);
    TEST_ASSERT_EQUAL(INVALID, reading(NAN, true, 0, 500).quality);
    TEST_ASSERT_EQUAL(INVALID, reading(INFINITY, true, 0, 500).quality);
}

void test_decimal_and_negative_yaw_output() {
    char text[16];
    TelemetryData::valueText(text, sizeof(text), {-12.25f, TelemetryData::VALID}, 2);
    TEST_ASSERT_EQUAL_STRING("-12.25", text);
    TelemetryData::valueText(text, sizeof(text), {12.25f, TelemetryData::VALID}, 1);
    TEST_ASSERT_EQUAL_STRING("12.3", text);
    TelemetryData::valueText(text, sizeof(text), {-0.125f, TelemetryData::VALID}, 3);
    TEST_ASSERT_EQUAL_STRING("-0.125", text);
    TelemetryData::valueText(text, sizeof(text), {12.25f, TelemetryData::VALID}, 0);
    TEST_ASSERT_EQUAL_STRING("12", text);
    TelemetryData::valueText(text, sizeof(text), {0.0f, TelemetryData::VALID}, 2);
    TEST_ASSERT_EQUAL_STRING("0.00", text);
    TelemetryData::valueText(text, sizeof(text), {0, TelemetryData::INVALID}, 1);
    TEST_ASSERT_EQUAL_STRING("INV", text);
    TelemetryData::valueText(text, sizeof(text), {0, TelemetryData::STALE}, 1);
    TEST_ASSERT_EQUAL_STRING("STALE", text);
}

void test_front_right_left_order() {
    TelemetryData::Snapshot s = {};
    s.timestamp = 1234;
    s.front = {30.0f, TelemetryData::VALID};
    s.right = {0, TelemetryData::INVALID};
    s.left = {0, TelemetryData::STALE};
    char line[64];
    TEST_ASSERT_NOT_EQUAL(0, TelemetryData::format(line, sizeof(line), s, 0, 1, 2));
    TEST_ASSERT_EQUAL_STRING("[1234 ms] TEL D cm F=30.0 R=INV L=STALE\n", line);
}

void test_extreme_records_fit_uart_buffer() {
    TelemetryData::Snapshot s = {};
    s.timestamp = s.encoderLeft = s.encoderRight = 0xFFFFFFFFUL;
    s.front = s.left = s.right = {400.0f, TelemetryData::VALID};
    s.gyroZ = {-500.0f, TelemetryData::VALID};
    s.motion = 2; s.pwmLeft = s.pwmRight = 255;
    s.fault = 1; s.stopped = s.pending = true; s.gyroState = 0;
    char line[64];
    for (uint8_t part = 0; part < TelemetryData::PARTS; ++part) {
        const size_t size = TelemetryData::format(line, sizeof(line), s, part, 3, 3);
        TEST_ASSERT_TRUE(size > 0 && size <= 63);
        TEST_ASSERT_EQUAL_UINT32(size, strlen(line));
        TEST_ASSERT_EQUAL_CHAR('\n', line[size - 1]);
    }
    TEST_ASSERT_NOT_NULL(strstr(line, "fault=ENCODER stop=1 pend=1 gyro=MISSING"));
}

void test_truncated_records_are_rejected() {
    TelemetryData::Snapshot s = {};
    char line[8] = {};
    TEST_ASSERT_EQUAL(0, TelemetryData::format(line, sizeof(line), s, 1, 1, 2));
    TEST_ASSERT_EQUAL_STRING("", line);
    char full[64];
    TEST_ASSERT_EQUAL(0, TelemetryData::format(full, sizeof(full), s, 9, 1, 2));
}

struct FakeSerial {
    int freeBytes;
    unsigned calls = 0;
    size_t written = 0;
    int availableForWrite() { return freeBytes; }
    size_t write(const uint8_t *, size_t length) { ++calls; written += length; return length; }
};
void test_full_buffer_never_writes_or_partially_writes() {
    FakeSerial serial = {0};
    TEST_ASSERT_FALSE(LogOutput::tryWrite(serial, "hello\n", 6));
    serial.freeBytes = 5;
    TEST_ASSERT_FALSE(LogOutput::tryWrite(serial, "hello\n", 6));
    TEST_ASSERT_EQUAL_UINT32(0, serial.calls);
    serial.freeBytes = 6;
    TEST_ASSERT_TRUE(LogOutput::tryWrite(serial, "hello\n", 6));
    TEST_ASSERT_EQUAL_UINT32(1, serial.calls);
    TEST_ASSERT_EQUAL_UINT32(6, serial.written);
}

void test_millis_rollover_elapsed() {
    TEST_ASSERT_EQUAL_UINT32(32, Clock::elapsedMs(16, 0xFFFFFFF0UL));
}

void test_mode_fault_and_gyro_status() {
    TelemetryData::Snapshot s = {};
    char line[64];
    s.timestamp = 2000; s.motion = 1; s.pwmLeft = 130; s.pwmRight = 150; s.automatic = true;
    TelemetryData::format(line, sizeof(line), s, 3, 1, 2);
    TEST_ASSERT_EQUAL_STRING("[2000 ms] TEL M FORWARD pwm=130,150 AUTO\n", line);
    s.fault = 0; s.gyroState = 2; s.stopped = true;
    TelemetryData::format(line, sizeof(line), s, 4, 1, 2);
    TEST_ASSERT_EQUAL_STRING("[2000 ms] TEL S fault=GYRO stop=1 pend=0 gyro=FAULT\n", line);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_invalid_and_stale_readings);
    RUN_TEST(test_decimal_and_negative_yaw_output);
    RUN_TEST(test_front_right_left_order);
    RUN_TEST(test_extreme_records_fit_uart_buffer);
    RUN_TEST(test_truncated_records_are_rejected);
    RUN_TEST(test_full_buffer_never_writes_or_partially_writes);
    RUN_TEST(test_millis_rollover_elapsed);
    RUN_TEST(test_mode_fault_and_gyro_status);
    return UNITY_END();
}
