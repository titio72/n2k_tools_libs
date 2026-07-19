#include "SpeedSensorInterrupt.h"
#include "Utils.h"
#include <unity.h>

// ---------- construction / accessors ----------

void test_get_pin_returns_configured_pin()
{
    SpeedSensorInterrupt s(5);
    TEST_ASSERT_EQUAL(5, s.get_pin());
}

void test_initial_counter_is_zero()
{
    SpeedSensorInterrupt s(5);
    TEST_ASSERT_EQUAL(0, s.get_counter());
}

void test_default_alpha_is_one()
{
    SpeedSensorInterrupt s(5);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, s.get_alpha());
}

void test_set_alpha_updates_alpha()
{
    SpeedSensorInterrupt s(5);
    s.set_alpha(0.3);
    TEST_ASSERT_EQUAL_DOUBLE(0.3, s.get_alpha());
}

// ---------- setup() / loop_micros() are no-ops under NATIVE ----------

void test_setup_and_loop_micros_do_not_crash_or_change_state()
{
    SpeedSensorInterrupt s(5);
    s.setup();
    s.loop_micros(12345);
    TEST_ASSERT_EQUAL(0, s.get_counter());
}

// ---------- signal() debounce ----------

void test_signal_increments_counter_on_first_call()
{
    SpeedSensorInterrupt s(5);
    s.signal();
    TEST_ASSERT_EQUAL(1, s.get_counter());
}

void test_signal_debounces_immediate_repeated_calls()
{
    SpeedSensorInterrupt s(5);
    s.signal();
    s.signal(); // fired well within the 10ms debounce window
    s.signal();
    TEST_ASSERT_EQUAL(1, s.get_counter());
}

void test_signal_counts_again_after_debounce_period_elapses()
{
    SpeedSensorInterrupt s(5);
    s.signal();
    TEST_ASSERT_EQUAL(1, s.get_counter());

    msleep(15); // exceeds the 10ms debounce window
    s.signal();
    TEST_ASSERT_EQUAL(2, s.get_counter());
}

// ---------- read_data() ----------

void test_read_data_returns_false_before_50ms_elapsed()
{
    SpeedSensorInterrupt s(5);
    double freq = -1;
    int counter_out = -1;

    bool result = s.read_data(10, freq, counter_out);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, counter_out);
}

void test_read_data_returns_true_and_resets_counter_after_50ms()
{
    SpeedSensorInterrupt s(5);
    s.signal(); // counter = 1

    double freq = 0;
    int counter_out = 0;
    bool result = s.read_data(60, freq, counter_out);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, counter_out);
    TEST_ASSERT_EQUAL(0, s.get_counter());
    // alpha=1.0 -> smooth_counter == counter; frequency = counter*1000/dt/2
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1000.0 / 60.0 / 2.0, freq);
}

void test_read_data_ignores_negative_pin()
{
    SpeedSensorInterrupt s(-1);
    double freq = 0;
    int counter_out = 0;

    bool result = s.read_data(1000, freq, counter_out);

    TEST_ASSERT_FALSE(result);
}

void test_read_data_applies_alpha_smoothing_across_cycles()
{
    SpeedSensorInterrupt s(5);
    s.set_alpha(0.5);

    s.signal(); // counter = 1
    double freq = 0;
    int counter_out = 0;
    s.read_data(60, freq, counter_out);
    // smooth_counter = 1*0.5 + 0*0.5 = 0.5 -> freq = 0.5*1000/60/2
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.5 * 1000.0 / 60.0 / 2.0, freq);

    msleep(15); // exceeds the signal() debounce window so this signal is not swallowed
    s.signal(); // counter = 1 again
    s.read_data(120, freq, counter_out);
    // smooth_counter = 1*0.5 + 0.5*0.5 = 0.75 -> freq = 0.75*1000/60/2
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 0.75 * 1000.0 / 60.0 / 2.0, freq);
}

void test_get_sample_age_tracks_last_read_time()
{
    SpeedSensorInterrupt s(5);
    double freq = 0;
    int counter_out = 0;

    s.read_data(500, freq, counter_out);

    TEST_ASSERT_EQUAL(500, s.get_sample_age());
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_get_pin_returns_configured_pin);
    RUN_TEST(test_initial_counter_is_zero);
    RUN_TEST(test_default_alpha_is_one);
    RUN_TEST(test_set_alpha_updates_alpha);

    RUN_TEST(test_setup_and_loop_micros_do_not_crash_or_change_state);

    RUN_TEST(test_signal_increments_counter_on_first_call);
    RUN_TEST(test_signal_debounces_immediate_repeated_calls);
    RUN_TEST(test_signal_counts_again_after_debounce_period_elapses);

    RUN_TEST(test_read_data_returns_false_before_50ms_elapsed);
    RUN_TEST(test_read_data_returns_true_and_resets_counter_after_50ms);
    RUN_TEST(test_read_data_ignores_negative_pin);
    RUN_TEST(test_read_data_applies_alpha_smoothing_across_cycles);
    RUN_TEST(test_get_sample_age_tracks_last_read_time);

    UNITY_END();
    return 0;
}
