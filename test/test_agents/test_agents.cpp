#include <unity.h>
#include "Agents.hpp"

struct Context {};

struct FakeAgent {
  AB_AGENT
  bool can_enable = true;
  bool enabled = false;
  int enables = 0, disables = 0, loops = 0, setups = 0;
};
void FakeAgent::setup(Context &) { setups++; }
void FakeAgent::enable(Context &) { enables++; if (can_enable) enabled = true; }
void FakeAgent::disable(Context &) { disables++; enabled = false; }
bool FakeAgent::is_enabled() { return enabled; }
void FakeAgent::loop(unsigned long, Context &) { loops++; }

void setUp() {}
void tearDown() {}

void test_enabled_agent_is_enabled_once_and_looped_every_cycle() {
  Context ctx; FakeAgent a; AgentSlot slot;
  for (int i = 0; i < 5; i++) handle_agent_loop(a, ctx, true, slot, 1000, "fake");
  TEST_ASSERT_EQUAL(1, a.enables);
  TEST_ASSERT_EQUAL(5, a.loops);
}

void test_agent_is_not_looped_when_not_requested() {
  Context ctx; FakeAgent a; AgentSlot slot;
  handle_agent_loop(a, ctx, false, slot, 1000, "fake");
  TEST_ASSERT_EQUAL(0, a.enables);
  TEST_ASSERT_EQUAL(0, a.loops);
}

void test_agent_is_disabled_when_request_drops() {
  Context ctx; FakeAgent a; AgentSlot slot;
  handle_agent_loop(a, ctx, true, slot, 1000, "fake");
  handle_agent_loop(a, ctx, false, slot, 2000, "fake");
  TEST_ASSERT_EQUAL(1, a.disables);
  TEST_ASSERT_FALSE(a.is_enabled());
  handle_agent_loop(a, ctx, false, slot, 3000, "fake");
  TEST_ASSERT_EQUAL(1, a.disables);  // not disabled twice
}

void test_failed_enable_retries_max_retry_times_then_waits_for_cooldown() {
  Context ctx; FakeAgent a; a.can_enable = false; AgentSlot slot;
  for (int i = 0; i < 6; i++) handle_agent_loop(a, ctx, true, slot, 1000, "fake");
  TEST_ASSERT_EQUAL(MAX_RETRY, a.enables);
  TEST_ASSERT_EQUAL(0, a.loops);
  TEST_ASSERT_EQUAL_UINT32(1000 + AGENT_RETRY_COOLDOWN_USEC, slot.retry_at);

  handle_agent_loop(a, ctx, true, slot, 1000 + AGENT_RETRY_COOLDOWN_USEC - 1, "fake");
  TEST_ASSERT_EQUAL(MAX_RETRY, a.enables);  // still cooling down

  handle_agent_loop(a, ctx, true, slot, 1000 + AGENT_RETRY_COOLDOWN_USEC, "fake");
  TEST_ASSERT_EQUAL(MAX_RETRY + 1, a.enables);  // exactly one more attempt
  handle_agent_loop(a, ctx, true, slot, 1000 + AGENT_RETRY_COOLDOWN_USEC + 1, "fake");
  TEST_ASSERT_EQUAL(MAX_RETRY + 1, a.enables);
}

void test_enable_that_finally_succeeds_resets_retry_state() {
  Context ctx; FakeAgent a; a.can_enable = false; AgentSlot slot;
  for (int i = 0; i < 4; i++) handle_agent_loop(a, ctx, true, slot, 1000, "fake");
  a.can_enable = true;
  handle_agent_loop(a, ctx, true, slot, 1000 + AGENT_RETRY_COOLDOWN_USEC, "fake");
  TEST_ASSERT_TRUE(a.is_enabled());
  TEST_ASSERT_EQUAL(0, slot.retry);
  TEST_ASSERT_EQUAL_UINT32(0, slot.retry_at);
  TEST_ASSERT_EQUAL(1, a.loops);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_enabled_agent_is_enabled_once_and_looped_every_cycle);
  RUN_TEST(test_agent_is_not_looped_when_not_requested);
  RUN_TEST(test_agent_is_disabled_when_request_drops);
  RUN_TEST(test_failed_enable_retries_max_retry_times_then_waits_for_cooldown);
  RUN_TEST(test_enable_that_finally_succeeds_resets_retry_state);
  return UNITY_END();
}
