
#include <unity.h>
#include "SafetyRules.h"

void setUp() {}
void tearDown() {}

// Stop commands must be recognized.
void test_stop_commands() {
    TEST_ASSERT_TRUE(
        SafetyRules::isStopCommand('S')
    );
    TEST_ASSERT_TRUE(
        SafetyRules::isStopCommand('M')
    );
    TEST_ASSERT_FALSE(
        SafetyRules::isStopCommand('B')
    );
    TEST_ASSERT_FALSE(
        SafetyRules::isStopCommand('C')
    );
}

// Manual commands must expire after 1500 ms.
void test_manual_timeout() {
    TEST_ASSERT_FALSE(
        SafetyRules::manualExpired(1500, 0, 1500)
    );

    TEST_ASSERT_TRUE(
        SafetyRules::manualExpired(1501, 0, 1500)
    );
}

// Millis rollover must not break timeout math.
void test_timer_overflow() {
    TEST_ASSERT_FALSE(
        SafetyRules::manualExpired(
            10, 0xFFFFFFF0UL, 1500
        )
    );

    TEST_ASSERT_TRUE(
        SafetyRules::manualExpired(
            1510, 0xFFFFFFF0UL, 1500
        )
    );
}

// Safe distance must be recent and valid.
void test_front_distance() {
    TEST_ASSERT_TRUE(
        SafetyRules::frontClear(30, 100, 20, 450)
    );

    TEST_ASSERT_FALSE(
        SafetyRules::frontClear(10, 100, 20, 450)
    );

    TEST_ASSERT_FALSE(
        SafetyRules::frontClear(-1, 100, 20, 450)
    );

    TEST_ASSERT_FALSE(
        SafetyRules::frontClear(30, 451, 20, 450)
    );
}

// No encoder pulses must eventually cause a fault.
void test_encoder_stall() {
    // Still inside startup grace period.
    TEST_ASSERT_FALSE(
        SafetyRules::encoderStalled(
            500, 0, 0, 0, 700, 1200
        )
    );

    // Both wheels recently reported pulses.
    TEST_ASSERT_FALSE(
        SafetyRules::encoderStalled(
            1300, 0, 1200, 1250, 700, 1200
        )
    );

    // Left encoder has stopped reporting pulses.
    TEST_ASSERT_TRUE(
        SafetyRules::encoderStalled(
            1300, 0, 0, 1250, 700, 1200
        )
    );

    // Right encoder has stopped reporting pulses.
    TEST_ASSERT_TRUE(
        SafetyRules::encoderStalled(
            1300, 0, 1250, 0, 700, 1200
        )
    );
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_stop_commands);
    RUN_TEST(test_manual_timeout);
    RUN_TEST(test_timer_overflow);
    RUN_TEST(test_front_distance);
    RUN_TEST(test_encoder_stall);

    return UNITY_END();
}
