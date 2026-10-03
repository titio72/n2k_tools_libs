#include <unity.h>
#include <string>
#include <vector>
#include <cstring>
#include "BTInterface.h"

struct Recorder : ABBLEWriteCallback {
  int handle = -1;
  std::string text;
  int calls = 0;
  void on_write(int h, const char* v) override { handle = h; text = v; calls++; }
};

struct RawRecorder : Recorder {
  std::vector<uint8_t> raw;
  void on_write_bytes(int h, const uint8_t* d, size_t n) override { handle = h; raw.assign(d, d + n); calls++; }
};

void setUp() {}
void tearDown() {}

void test_default_on_write_bytes_falls_back_to_string_and_stops_at_nul() {
  Recorder r;
  const uint8_t data[] = {'a', 'b', 0, 'c'};
  r.on_write_bytes(3, data, sizeof data);
  TEST_ASSERT_EQUAL(1, r.calls);
  TEST_ASSERT_EQUAL(3, r.handle);
  TEST_ASSERT_EQUAL_STRING("ab", r.text.c_str());
}

void test_default_on_write_bytes_truncates_at_255() {
  Recorder r;
  std::vector<uint8_t> big(300, 'x');
  r.on_write_bytes(0, big.data(), big.size());
  TEST_ASSERT_EQUAL(255, r.text.size());
}

void test_default_on_write_bytes_empty_write_gives_empty_string() {
  Recorder r;
  r.on_write_bytes(1, nullptr, 0);
  TEST_ASSERT_EQUAL(1, r.calls);
  TEST_ASSERT_EQUAL_STRING("", r.text.c_str());
}

void test_override_receives_binary_payload_with_zero_bytes() {
  RawRecorder r;
  const uint8_t data[] = {1, 0, 2, 0, 0};
  r.on_write_bytes(0, data, sizeof data);
  TEST_ASSERT_EQUAL(5, r.raw.size());
  TEST_ASSERT_EQUAL_HEX8_ARRAY(data, r.raw.data(), 5);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_default_on_write_bytes_falls_back_to_string_and_stops_at_nul);
  RUN_TEST(test_default_on_write_bytes_truncates_at_255);
  RUN_TEST(test_default_on_write_bytes_empty_write_gives_empty_string);
  RUN_TEST(test_override_receives_binary_payload_with_zero_bytes);
  return UNITY_END();
}
