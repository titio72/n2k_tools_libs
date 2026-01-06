#include "Log.h"

#include <vector>

#ifndef NATIVE
#include <Arduino.h>
#endif

#pragma region timers_management
#ifndef TACHO_TIMER_N
#define TACHO_TIMER_N 0
#endif
#define TACHO_TIMER_DIVIDER 80
#define TACHO_TIMER_ALARM_VALUE 100

#include <SpeedSensor.h>

std::vector<SpeedSensor*> _tach = {};

void add_tacho(SpeedSensor* tachometer)
{
    for (int i = 0; i<_tach.size(); i++)
    {
        if (_tach[i]==NULL)
        {
            _tach[i] = tachometer;
            return;
        }
    }
    _tach.push_back(tachometer);
}

void remove_tacho(SpeedSensor* tachometer)
{
    for (int i = 0; i<_tach.size(); i++)
    {
        if (_tach[i]==tachometer) _tach[i] = NULL;
    }
}

bool contains_tacho(const SpeedSensor* tachometer)
{
        for (int i = 0; i<_tach.size(); i++)
    {
        if (_tach[i]==tachometer) return true;
    }
    return false;
}

#ifndef NATIVE
static hw_timer_t* timer = NULL;

void IRAM_ATTR on_timer()
{
    unsigned long now_micros = micros();
    for (int i = 0; i<_tach.size(); i++)
    {
        if (_tach[i])
        {
            _tach[i]->loop_micros(now_micros);
        }
    }
}
#endif

void init_timer()
{
    #ifndef PIO_UNIT_TESTING
    #ifndef NATIVE
    if (timer==NULL)
    {
        timer = timerBegin(TACHO_TIMER_N, TACHO_TIMER_DIVIDER, true); // 10Khz
        timerAttachInterrupt(timer, on_timer, true);
        timerAlarmWrite(timer, TACHO_TIMER_ALARM_VALUE, true);
        timerAlarmEnable(timer);
        Log::tracex("Timer", "Set timer", "Timer {%d} initialized at {%d Hz}", TACHO_TIMER_N, getCpuFrequencyMhz() * 1000L * TACHO_TIMER_ALARM_VALUE / TACHO_TIMER_DIVIDER);
    }
    #endif
    #endif
}