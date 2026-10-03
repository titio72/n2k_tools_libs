#include <unity.h>
#include "N2K.h"

static bool match_any(const n2k_device_summary&) { return true; }

void setUp() {}
void tearDown() {}

void test_device_info_default_load_equivalency_is_one() {
  n2k_device_info info;
  TEST_ASSERT_EQUAL_UINT8(1, info.LoadEquivalency);
}

void test_find_device_is_false_without_a_device_list() {
  N2K* n2k = N2K::get_instance(nullptr, nullptr);
  unsigned char src = 77;
  TEST_ASSERT_FALSE(n2k->find_device(match_any, src));
  TEST_ASSERT_EQUAL_UINT8(77, src);  // untouched
}

void test_find_device_with_null_matcher_is_false() {
  N2K* n2k = N2K::get_instance(nullptr, nullptr);
  unsigned char src = 77;
  TEST_ASSERT_FALSE(n2k->find_device(nullptr, src));
}

void test_new_options_are_safe_on_the_host_where_there_is_no_bus() {
  N2K* n2k = N2K::get_instance(nullptr, nullptr);
  n2k->set_listen_all(true);
  n2k->add_rx_pgn(65379);
  n2k->enable_device_list();
  n2k_device_info info;
  info.LoadEquivalency = 2;
  n2k->setup(info);
  TEST_ASSERT_FALSE(n2k->is_initialized());  // no CAN driver on the host
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_device_info_default_load_equivalency_is_one);
  RUN_TEST(test_find_device_is_false_without_a_device_list);
  RUN_TEST(test_find_device_with_null_matcher_is_false);
  RUN_TEST(test_new_options_are_safe_on_the_host_where_there_is_no_bus);
  return UNITY_END();
}
