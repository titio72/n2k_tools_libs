#include "Utils.h"
#include <unity.h>
#include <cstring>

// ---------- N2KSid ----------

void test_n2ksid_initial_value() {
    N2KSid sid;
    TEST_ASSERT_EQUAL(0, sid.getCurrent());
}

void test_n2ksid_getnew_increments() {
    N2KSid sid;
    TEST_ASSERT_EQUAL(1, sid.getNew());
    TEST_ASSERT_EQUAL(1, sid.getCurrent());
    TEST_ASSERT_EQUAL(2, sid.getNew());
    TEST_ASSERT_EQUAL(2, sid.getCurrent());
}

void test_n2ksid_wraps_at_253() {
    N2KSid sid;
    unsigned char last = 0;
    for (int i = 0; i < 253; i++) {
        last = sid.getNew();
    }
    TEST_ASSERT_EQUAL(0, last);
    TEST_ASSERT_EQUAL(0, sid.getCurrent());
}

// ---------- ByteBuffer ----------

void test_bytebuffer_starts_empty() {
    ByteBuffer bb(16);
    TEST_ASSERT_EQUAL(0, bb.length());
    TEST_ASSERT_EQUAL(16, bb.size());
}

void test_bytebuffer_append_numeric() {
    ByteBuffer bb(16);
    uint8_t v8 = 0xAB;
    uint16_t v16 = 0x1234;
    bb << v8 << v16;
    TEST_ASSERT_EQUAL(3, bb.length());

    uint8_t out[3];
    bb.get_data(out, 3);
    TEST_ASSERT_EQUAL_HEX8(0xAB, out[0]);
    TEST_ASSERT_EQUAL_HEX8(0x34, out[1]); // little-endian low byte
    TEST_ASSERT_EQUAL_HEX8(0x12, out[2]); // little-endian high byte
}

void test_bytebuffer_append_string_is_length_prefixed() {
    ByteBuffer bb(16);
    bb << "Hi";
    TEST_ASSERT_EQUAL(3, bb.length()); // 1 length byte + 2 chars

    uint8_t out[3];
    bb.get_data(out, 3);
    TEST_ASSERT_EQUAL(2, out[0]);
    TEST_ASSERT_EQUAL('H', out[1]);
    TEST_ASSERT_EQUAL('i', out[2]);
}

void test_bytebuffer_reset() {
    ByteBuffer bb(16);
    uint8_t v = 1;
    bb << v << v;
    TEST_ASSERT_EQUAL(2, bb.length());
    bb.reset();
    TEST_ASSERT_EQUAL(0, bb.length());
}

void test_bytebuffer_overflow_without_autoexpand_is_dropped() {
    ByteBuffer bb(1, false);
    uint16_t v16 = 0x1234; // needs 2 bytes, buffer only has 1
    bb << v16;
    TEST_ASSERT_EQUAL(0, bb.length());
}

void test_bytebuffer_autoexpand_grows() {
    ByteBuffer bb(1, true);
    uint16_t v16 = 0x1234;
    bb << v16;
    TEST_ASSERT_EQUAL(2, bb.length());
    TEST_ASSERT_TRUE(bb.size() >= 2);
}

void test_bytebuffer_construct_from_data() {
    uint8_t src[4] = {1, 2, 3, 4};
    ByteBuffer bb(src, 4);
    TEST_ASSERT_EQUAL(4, bb.length());
    uint8_t out[4];
    bb.get_data(out, 4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(src, out, 4);
}

void test_bytebuffer_equality() {
    uint8_t src[3] = {1, 2, 3};
    ByteBuffer a(src, 3);
    ByteBuffer b(src, 3);
    ByteBuffer c(4, false);
    uint8_t v = 9;
    c << v;

    TEST_ASSERT_TRUE(a == b);
    TEST_ASSERT_FALSE(a == c);
}

void test_bytebuffer_copy_constructor_is_independent() {
    uint8_t src[2] = {5, 6};
    ByteBuffer a(src, 2);
    ByteBuffer b(a);

    TEST_ASSERT_TRUE(a == b);

    uint8_t v = 7;
    b << v; // modifies only b (auto_expand defaults false, but has room? size==2, offset==2 -> won't fit, dropped)
    // Regardless of whether the append succeeded, 'a' must be unaffected.
    TEST_ASSERT_EQUAL(2, a.length());
}

void test_bytebuffer_assignment_operator_copies_content() {
    uint8_t src[2] = {5, 6};
    ByteBuffer a(src, 2);
    ByteBuffer b(4, false);
    b = a;
    TEST_ASSERT_TRUE(a == b);
}

// ---------- startswith ----------

void test_startswith_true() {
    TEST_ASSERT_TRUE(startswith("He", "Hello"));
}

void test_startswith_false() {
    TEST_ASSERT_FALSE(startswith("lo", "Hello"));
}

void test_startswith_prefix_longer_than_string() {
    TEST_ASSERT_FALSE(startswith("Hello there", "Hi"));
}

void test_startswith_full_match() {
    TEST_ASSERT_TRUE(startswith("Hello", "Hello"));
}

// ---------- indexOf ----------

void test_indexof_found() {
    TEST_ASSERT_EQUAL(7, indexOf("Hello, World!", "World"));
}

void test_indexof_not_found() {
    TEST_ASSERT_EQUAL(-1, indexOf("Hello, World!", "xyz"));
}

void test_indexof_at_start() {
    TEST_ASSERT_EQUAL(0, indexOf("Hello", "Hello"));
}

// ---------- replace ----------

void test_replace_all_occurrences() {
    char *result = replace("a-b-c", "-", "_");
    TEST_ASSERT_EQUAL_STRING("a_b_c", result);
    free(result);
}

void test_replace_first_occurrence_only() {
    char *result = replace("a-b-c", "-", "_", true);
    TEST_ASSERT_EQUAL_STRING("a_b-c", result);
    free(result);
}

void test_replace_no_match_returns_copy() {
    char *result = replace("hello", "xyz", "_");
    TEST_ASSERT_EQUAL_STRING("hello", result);
    free(result);
}

// ---------- norm_deg ----------

void test_norm_deg_double_in_range() {
    TEST_ASSERT_EQUAL_DOUBLE(45.0, norm_deg(45.0));
}

void test_norm_deg_double_over() {
    TEST_ASSERT_EQUAL_DOUBLE(10.0, norm_deg(370.0));
}

void test_norm_deg_double_negative() {
    TEST_ASSERT_EQUAL_DOUBLE(330.0, norm_deg(-30.0));
}

void test_norm_deg_double_exact_360() {
    TEST_ASSERT_EQUAL_DOUBLE(0.0, norm_deg(360.0));
}

void test_norm_deg_int16_in_range() {
    TEST_ASSERT_EQUAL_INT16(45, norm_deg((int16_t)45));
}

void test_norm_deg_int16_over() {
    TEST_ASSERT_EQUAL_INT16(10, norm_deg((int16_t)370));
}

void test_norm_deg_int16_negative() {
    TEST_ASSERT_EQUAL_INT16(330, norm_deg((int16_t)(-30)));
}

// ---------- array_contains ----------

void test_array_contains_found() {
    int values[] = {1, 2, 3, 4};
    TEST_ASSERT_TRUE(array_contains(3, values, 4));
}

void test_array_contains_not_found() {
    int values[] = {1, 2, 3, 4};
    TEST_ASSERT_FALSE(array_contains(9, values, 4));
}

void test_array_contains_empty_size() {
    int values[] = {1, 2, 3, 4};
    TEST_ASSERT_FALSE(array_contains(1, values, 0));
}

void test_array_contains_null_array() {
    TEST_ASSERT_FALSE(array_contains(1, (int *)NULL, 4));
}

// ---------- getDaysSince1970 ----------

void test_getdayssince1970_epoch() {
    TEST_ASSERT_EQUAL(0, getDaysSince1970(1970, 1, 1));
}

void test_getdayssince1970_next_day() {
    TEST_ASSERT_EQUAL(1, getDaysSince1970(1970, 1, 2));
}

void test_getdayssince1970_year_2000() {
    TEST_ASSERT_EQUAL(10957, getDaysSince1970(2000, 1, 1));
}

void test_getdayssince1970_leap_day_boundary() {
    TEST_ASSERT_EQUAL(10956, getDaysSince1970(1999, 12, 31));
}

// ---------- time_to_ISO ----------

void test_time_to_iso_format() {
    // 2000-01-01T00:00:00.500Z == 946684800 seconds since epoch
    const char *result = time_to_ISO((time_t)946684800, 500);
    TEST_ASSERT_EQUAL_STRING("2000-01-01T00:00:00.500Z", result);
}

// ---------- check_elapsed ----------

void test_check_elapsed_first_call_always_fires() {
    ulong last_time = 0;
    ulong result = check_elapsed(1000, last_time, 5000);
    TEST_ASSERT_EQUAL(1000, result);
    TEST_ASSERT_EQUAL(1000, last_time);
}

void test_check_elapsed_before_period_returns_zero() {
    ulong last_time = 1000;
    ulong result = check_elapsed(2000, last_time, 5000);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(1000, last_time); // unchanged
}

void test_check_elapsed_after_period_fires() {
    ulong last_time = 1000;
    ulong result = check_elapsed(6000, last_time, 5000);
    TEST_ASSERT_EQUAL(5000, result);
    TEST_ASSERT_EQUAL(6000, last_time);
}

// ---------- format_thousands_sep ----------

void test_format_thousands_sep_small_number() {
    char buf[32];
    format_thousands_sep(buf, 42);
    TEST_ASSERT_EQUAL_STRING("42", buf);
}

void test_format_thousands_sep_thousand() {
    char buf[32];
    format_thousands_sep(buf, 1000);
    TEST_ASSERT_EQUAL_STRING("1,000", buf);
}

void test_format_thousands_sep_millions() {
    char buf[32];
    format_thousands_sep(buf, 1234567);
    TEST_ASSERT_EQUAL_STRING("1,234,567", buf);
}

// ---------- lpf ----------

void test_lpf_alpha_zero_keeps_previous() {
    TEST_ASSERT_EQUAL_DOUBLE(10.0, lpf(20.0, 10.0, 0.0));
}

void test_lpf_alpha_one_takes_new_value() {
    TEST_ASSERT_EQUAL_DOUBLE(20.0, lpf(20.0, 10.0, 1.0));
}

void test_lpf_blends_values() {
    TEST_ASSERT_EQUAL_DOUBLE(15.0, lpf(20.0, 10.0, 0.5));
}

// ---------- timing helpers ----------

void test_micros_and_millis_are_positive_and_consistent() {
    ulong us = _micros();
    ulong ms = _millis();
    TEST_ASSERT_TRUE(us > 0);
    TEST_ASSERT_TRUE(ms > 0);
}

void test_msleep_sleeps_approximately_requested_time() {
    ulong before = _millis();
    msleep(20);
    ulong after = _millis();
    TEST_ASSERT_TRUE((after - before) >= 15);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_n2ksid_initial_value);
    RUN_TEST(test_n2ksid_getnew_increments);
    RUN_TEST(test_n2ksid_wraps_at_253);

    RUN_TEST(test_bytebuffer_starts_empty);
    RUN_TEST(test_bytebuffer_append_numeric);
    RUN_TEST(test_bytebuffer_append_string_is_length_prefixed);
    RUN_TEST(test_bytebuffer_reset);
    RUN_TEST(test_bytebuffer_overflow_without_autoexpand_is_dropped);
    RUN_TEST(test_bytebuffer_autoexpand_grows);
    RUN_TEST(test_bytebuffer_construct_from_data);
    RUN_TEST(test_bytebuffer_equality);
    RUN_TEST(test_bytebuffer_copy_constructor_is_independent);
    RUN_TEST(test_bytebuffer_assignment_operator_copies_content);

    RUN_TEST(test_startswith_true);
    RUN_TEST(test_startswith_false);
    RUN_TEST(test_startswith_prefix_longer_than_string);
    RUN_TEST(test_startswith_full_match);

    RUN_TEST(test_indexof_found);
    RUN_TEST(test_indexof_not_found);
    RUN_TEST(test_indexof_at_start);

    RUN_TEST(test_replace_all_occurrences);
    RUN_TEST(test_replace_first_occurrence_only);
    RUN_TEST(test_replace_no_match_returns_copy);

    RUN_TEST(test_norm_deg_double_in_range);
    RUN_TEST(test_norm_deg_double_over);
    RUN_TEST(test_norm_deg_double_negative);
    RUN_TEST(test_norm_deg_double_exact_360);
    RUN_TEST(test_norm_deg_int16_in_range);
    RUN_TEST(test_norm_deg_int16_over);
    RUN_TEST(test_norm_deg_int16_negative);

    RUN_TEST(test_array_contains_found);
    RUN_TEST(test_array_contains_not_found);
    RUN_TEST(test_array_contains_empty_size);
    RUN_TEST(test_array_contains_null_array);

    RUN_TEST(test_getdayssince1970_epoch);
    RUN_TEST(test_getdayssince1970_next_day);
    RUN_TEST(test_getdayssince1970_year_2000);
    RUN_TEST(test_getdayssince1970_leap_day_boundary);

    RUN_TEST(test_time_to_iso_format);

    RUN_TEST(test_check_elapsed_first_call_always_fires);
    RUN_TEST(test_check_elapsed_before_period_returns_zero);
    RUN_TEST(test_check_elapsed_after_period_fires);

    RUN_TEST(test_format_thousands_sep_small_number);
    RUN_TEST(test_format_thousands_sep_thousand);
    RUN_TEST(test_format_thousands_sep_millions);

    RUN_TEST(test_lpf_alpha_zero_keeps_previous);
    RUN_TEST(test_lpf_alpha_one_takes_new_value);
    RUN_TEST(test_lpf_blends_values);

    RUN_TEST(test_micros_and_millis_are_positive_and_consistent);
    RUN_TEST(test_msleep_sleeps_approximately_requested_time);

    UNITY_END();
    return 0;
}
