#ifndef N2K_TOOLS_AGENTS_HPP
#define N2K_TOOLS_AGENTS_HPP

// Agent pattern shared by the applications built on n2k_tools_libs.
//
// An agent implements the five methods declared by AB_AGENT. The application must declare a type named
// `Context` (whatever it wants to hand to its agents) before expanding the macro. handle_agent_loop() enables
// and disables an agent from a runtime flag, retries a failing enable (MAX_RETRY quick attempts, then one
// attempt every AGENT_RETRY_COOLDOWN_USEC) and measures the time spent in the agent.

#include "Log.h"

#ifndef APP_LOG_TAG
#define APP_LOG_TAG "APP"
#endif
#ifndef MAX_RETRY
#define MAX_RETRY 3 // number of quick retries to start an agent
#endif
#ifndef AGENT_RETRY_COOLDOWN_USEC
#define AGENT_RETRY_COOLDOWN_USEC 30000000UL // then one more attempt every 30 s
#endif

#ifdef NATIVE
#define NOW_MICROS 0
#else
#include <Arduino.h>
#define NOW_MICROS micros()
#endif

#define AB_AGENT \
void enable(Context &ctx); \
void disable(Context &ctx); \
bool is_enabled(); \
void loop(unsigned long time, Context &ctx ); \
void setup(Context &ctx); \

// Per-agent bookkeeping: enable-retry state and loop time accumulated since the last stats dump
struct AgentSlot
{
  unsigned short retry = 0;
  unsigned long retry_at = 0;
  unsigned long loop_time = 0;
};

template <typename T, typename Ctx>
bool handle_agent_enable(T &agent, Ctx &ctx, unsigned short *retry, const char *desc,
                         unsigned long now_micros, unsigned long *retry_at)
{
  if (!agent.is_enabled())
  {
    if (retry && retry_at && (*retry) >= MAX_RETRY && (*retry_at) != 0 && (long)(now_micros - (*retry_at)) >= 0)
    {
      (*retry) = MAX_RETRY - 1; // one more attempt
    }

    if (retry == NULL || (*retry) < MAX_RETRY)
    {
      agent.enable(ctx);
      if (agent.is_enabled())
      {
        if (retry)
          (*retry) = 0;
        if (retry_at)
          (*retry_at) = 0;
      }
      else if (retry)
      {
        (*retry)++;
        if ((*retry) >= MAX_RETRY)
        {
          if (retry_at)
          {
            (*retry_at) = now_micros + AGENT_RETRY_COOLDOWN_USEC;
            if ((*retry_at) == 0) (*retry_at) = 1; // 0 means "not scheduled"
          }
          Log::tracex(APP_LOG_TAG, "Exceeded enable retry", "Module {%s}", desc);
        }
      }
    }
  }
  return agent.is_enabled();
}

template <typename T, typename Ctx>
unsigned long handle_agent_loop(T &agent, Ctx &ctx, bool enable, unsigned short *retry, unsigned long now_micros,
                                const char *desc = "", unsigned long *retry_at = NULL)
{
  unsigned long t = NOW_MICROS;
  if (enable)
  {
    if (handle_agent_enable(agent, ctx, retry, desc, now_micros, retry_at))
    {
      agent.loop(now_micros, ctx);
    }
  }
  else if (agent.is_enabled())
  {
    agent.disable(ctx);
    if (retry)
      (*retry) = 0;
    if (retry_at)
      (*retry_at) = 0;
  }
  return NOW_MICROS - t;
}

/** Same as above, keeping the retry state in the agent's slot and adding the time spent to slot.loop_time. */
template <typename T, typename Ctx>
unsigned long handle_agent_loop(T &agent, Ctx &ctx, bool enable, AgentSlot &slot, unsigned long now_micros,
                                const char *desc = "")
{
  unsigned long elapsed = handle_agent_loop(agent, ctx, enable, &slot.retry, now_micros, desc, &slot.retry_at);
  slot.loop_time += elapsed;
  return elapsed;
}

#endif
