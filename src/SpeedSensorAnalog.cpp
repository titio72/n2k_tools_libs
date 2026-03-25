#ifndef NATIVE
#include <Arduino.h>
#endif
#include "SpeedSensorAnalog.h"
#include "Utils.h"

SpeedSensorAnalog::SpeedSensorAnalog(int p) : pin(p), counter(0), state(LOW)
{
}

SpeedSensorAnalog::~SpeedSensorAnalog()
{
}

bool SpeedSensorAnalog::read_data(unsigned long milliseconds, double &frequency, int& counter_out)
{
    counter_out = counter;

    unsigned long dt = milliseconds - last_read_time;
    last_read_time = milliseconds;

    if (dt > 50 && pin >= 0) // arbitrary 50ms interval between two readings to avoid bouncing and too frequent readings
    {
        smooth_counter = (double)counter * alpha + smooth_counter * (1.0 - alpha);
        frequency = smooth_counter * 1000.0 / (double)dt / 2.0; // in Hz
        
        counter = 0;
        return true;
    }
    else
    {
        return false;
    }
}

// the time is in micros! called from an ISR every 1ms
void SpeedSensorAnalog::loop_micros(unsigned long t)
{
    #ifndef NATIVE
    if (pin >= 0)
    {
        uint16_t value = analogRead(pin);
        max = (value > max) ? value : max;
        min = (value < min) ? value : min;
        int new_state = (value >= high_threshold) ? HIGH : ((value <= low_threshold) ? LOW : state); // add some hysteresis to avoid bouncing around the threshold value (~0.7-2.5V for a 10-bit ADC with 3.3V reference), if the value is between 1500 and 3000, keep the previous state
        read_signal(new_state, t);
    }
    #endif
}

void SpeedSensorAnalog::read_signal(int new_state, unsigned long t_micros)
{
    if (new_state != state)
    {
        counter++;
        state = new_state;
    }
}

void SpeedSensorAnalog::setup()
{
    #ifndef NATIVE
    if (pin >= 0)
    {
        pinMode(pin, INPUT);
    }
    #endif
}
