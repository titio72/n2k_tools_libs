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

struct MockState : InternalBLEState {
  std::vector<ABBLEField> fields;
  std::vector<ABBLESetting> settings;
  uint32_t passkey = 0, changed = 0;
  uint16_t cmin = 0, cmax = 0, clat = 0, cto = 0;
  void init(const char*, const char*, ABBLEWriteCallback*) override {}
  void setup(const std::vector<ABBLEField>& f, const std::vector<ABBLESetting>& s) override { fields = f; settings = s; }
  void begin() override {}
  void change_device_name(const char*) override {}
  const char* get_device_name() override { return "mock"; }
  void end() override {}
  void set_field_value(int, const char*) override {}
  void set_field_value(int, uint16_t) override {}
  void set_field_value(int, void*, int) override {}
  ByteBuffer get_field_value(int) override { return ByteBuffer(0); }
  void set_setting_value(int, const char*) override {}
  void set_setting_value(int, int) override {}
  void set_passkey(uint32_t p) override { passkey = p; }
  void change_passkey(uint32_t p) override { changed = p; }
  void set_conn_params(uint16_t a, uint16_t b, uint16_t c, uint16_t d) override { cmin = a; cmax = b; clat = c; cto = d; }
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

void test_field_and_setting_defaults_keep_historic_behaviour() {
  Recorder cb; MockState st;
  BTInterface ble("uuid", "name", &cb, &st);
  TEST_ASSERT_EQUAL(0, ble.add_field("f", "u1"));
  TEST_ASSERT_EQUAL(0, ble.add_setting("s", "u2"));
  ble.setup();
  TEST_ASSERT_FALSE(st.fields[0].notify);
  TEST_ASSERT_FALSE(st.fields[0].secured_read);
  TEST_ASSERT_TRUE(st.settings[0].secured);
  TEST_ASSERT_FALSE(st.settings[0].secured_read);
}

void test_notify_and_secured_read_flags_reach_the_state() {
  Recorder cb; MockState st;
  BTInterface ble("uuid", "name", &cb, &st);
  ble.add_field("a", "u1", true, true);
  ble.add_field("b", "u2", true, false);
  ble.add_setting("c", "u3", true, true);
  ble.setup();
  TEST_ASSERT_TRUE(st.fields[0].notify);
  TEST_ASSERT_TRUE(st.fields[0].secured_read);
  TEST_ASSERT_TRUE(st.fields[1].notify);
  TEST_ASSERT_FALSE(st.fields[1].secured_read);
  TEST_ASSERT_TRUE(st.settings[0].secured_read);
}

void test_conn_params_and_passkey_are_forwarded() {
  Recorder cb; MockState st;
  BTInterface ble("uuid", "name", &cb, &st);
  ble.set_passkey(482913);
  ble.set_conn_params(24, 24, 0, 400);
  ble.change_passkey(111222);
  TEST_ASSERT_EQUAL_UINT32(482913, st.passkey);
  TEST_ASSERT_EQUAL_UINT32(111222, st.changed);
  TEST_ASSERT_EQUAL_UINT16(24, st.cmin);
  TEST_ASSERT_EQUAL_UINT16(24, st.cmax);
  TEST_ASSERT_EQUAL_UINT16(0, st.clat);
  TEST_ASSERT_EQUAL_UINT16(400, st.cto);
}

void test_set_conn_params_without_state_is_harmless() {
  Recorder cb;
  BTInterface ble("uuid", "name", &cb, nullptr);  // no real BLE on the host
  ble.set_conn_params(24, 24, 0, 400);
  TEST_PASS();
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_default_on_write_bytes_falls_back_to_string_and_stops_at_nul);
  RUN_TEST(test_default_on_write_bytes_truncates_at_255);
  RUN_TEST(test_default_on_write_bytes_empty_write_gives_empty_string);
  RUN_TEST(test_override_receives_binary_payload_with_zero_bytes);
  RUN_TEST(test_field_and_setting_defaults_keep_historic_behaviour);
  RUN_TEST(test_notify_and_secured_read_flags_reach_the_state);
  RUN_TEST(test_conn_params_and_passkey_are_forwarded);
  RUN_TEST(test_set_conn_params_without_state_is_harmless);
  return UNITY_END();
}
