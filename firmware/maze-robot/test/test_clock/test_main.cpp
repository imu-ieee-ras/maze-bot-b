#include <unity.h>
#include "Clock.h"

void setUp() {}
void tearDown() {}

void test_setup_time_and_window_average() {
    Clock::LoopClock clock;
    Clock::Sample sample = {};
    clock.begin(1200);
    TEST_ASSERT_FALSE(clock.tick(1700, 1000, sample));
    TEST_ASSERT_TRUE(clock.tick(2200, 1000, sample));
    TEST_ASSERT_EQUAL_UINT64(2200, sample.uptimeMs);
    TEST_ASSERT_EQUAL_UINT32(1000, sample.windowMs);
    TEST_ASSERT_EQUAL_UINT32(2, sample.loops);
    TEST_ASSERT_EQUAL_UINT32(2000, sample.milliHz);
}

void test_blocking_work_is_included() {
    Clock::LoopClock clock;
    Clock::Sample sample = {};
    clock.begin(0);
    TEST_ASSERT_FALSE(clock.tick(500, 1000, sample));
    TEST_ASSERT_TRUE(clock.tick(4500, 1000, sample));
    TEST_ASSERT_EQUAL_UINT32(4500, sample.windowMs);
    TEST_ASSERT_EQUAL_UINT32(444, sample.milliHz);
}

void test_next_window_excludes_previous_loops() {
    Clock::LoopClock clock;
    Clock::Sample sample = {};
    clock.begin(0);
    TEST_ASSERT_FALSE(clock.tick(500, 1000, sample));
    TEST_ASSERT_TRUE(clock.tick(1000, 1000, sample));
    TEST_ASSERT_TRUE(clock.tick(2000, 1000, sample));
    TEST_ASSERT_EQUAL_UINT32(1, sample.loops);
    TEST_ASSERT_EQUAL_UINT32(1000, sample.milliHz);
}

void test_millis_wrap_extends_uptime() {
    Clock::LoopClock clock;
    Clock::Sample sample = {};
    clock.begin(0xFFFFFFF0UL);
    TEST_ASSERT_TRUE(clock.tick(0x10UL, 32, sample));
    TEST_ASSERT_EQUAL_UINT64(0x100000010ULL, sample.uptimeMs);
    TEST_ASSERT_EQUAL_UINT32(32, sample.windowMs);
    TEST_ASSERT_EQUAL_UINT32(31250, sample.milliHz);
}

void test_zero_elapsed_and_disabled_interval() {
    Clock::LoopClock clock;
    Clock::Sample sample = {};
    clock.begin(10);
    TEST_ASSERT_FALSE(clock.tick(10, 1, sample));
    TEST_ASSERT_FALSE(clock.tick(11, 0, sample));
    TEST_ASSERT_TRUE(clock.tick(12, 2, sample));
    TEST_ASSERT_EQUAL_UINT32(3, sample.loops);
    TEST_ASSERT_EQUAL_UINT32(1500000, sample.milliHz);
}

void test_begin_resets_measurement() {
    Clock::LoopClock clock;
    Clock::Sample sample = {};
    clock.begin(0);
    clock.tick(1000, 1000, sample);
    clock.begin(5000);
    TEST_ASSERT_TRUE(clock.tick(6000, 1000, sample));
    TEST_ASSERT_EQUAL_UINT64(6000, sample.uptimeMs);
    TEST_ASSERT_EQUAL_UINT32(1, sample.loops);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_setup_time_and_window_average);
    RUN_TEST(test_blocking_work_is_included);
    RUN_TEST(test_next_window_excludes_previous_loops);
    RUN_TEST(test_millis_wrap_extends_uptime);
    RUN_TEST(test_zero_elapsed_and_disabled_interval);
    RUN_TEST(test_begin_resets_measurement);
    return UNITY_END();
}
