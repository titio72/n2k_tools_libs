#ifndef NATIVE
#include <Arduino.h>
#else
#define IRAM_ATTR
#endif
#include "SpeedSensorInterrupt.h"
#include "Utils.h"
#include "Log.h"

#pragma region Interrupt Handlers
/*
    Up to 4 speed sensors can be handled (n = 0..3)
    Note: on ESP32-C3 only 2 interrupts are available
*/

#define MAX_INSTANCES 4

typedef SpeedSensorInterrupt* SpeedSensorInterruptPtr;
static SpeedSensorInterruptPtr instances[] = {nullptr, nullptr, nullptr, nullptr};

#define SIGNAL_WRAPPER(n) \
    void IRAM_ATTR signal_wrapper_##n() \
    { \
        if (instances[n]) \
        { \
            instances[n]->signal(); \
        } \
    }

SIGNAL_WRAPPER(0)
SIGNAL_WRAPPER(1)
SIGNAL_WRAPPER(2)
SIGNAL_WRAPPER(3)

typedef void (*SignalWrapperType)();
static SignalWrapperType signal_wrappers[] = {signal_wrapper_0, signal_wrapper_1, signal_wrapper_2, signal_wrapper_3};

#pragma endregion

static int instance_count = 0;

SpeedSensorInterrupt::SpeedSensorInterrupt(int p, int n) : pin(p), n(n), counter(0)
{
    if (n < 0)
    {
        this->n = instance_count;
    }
    instance_count++;
}

SpeedSensorInterrupt::~SpeedSensorInterrupt()
{
}

bool SpeedSensorInterrupt::read_data(unsigned long milliseconds, double &frequency, int &counter_out)
{
    counter_out = counter;

    unsigned long dt = milliseconds - last_read_time;
    last_read_time = milliseconds;
    //Log::tracex("SPEED_SENSOR_INTERRUPT", "ReadData", "Pin %d Instance %d Counter %lu Dt %lu ms", pin, n, counter, dt);
    if (dt > 50 && pin >= 0) // arbitrary 50ms interval between two readings (it should be 250ms)
    {
        smooth_counter = (double)counter * alpha + smooth_counter * (1.0 - alpha);
        frequency = (smooth_counter * 1000.0 / (double)dt); // in Hz
        counter = 0;
        return true;
    }
    else
    {
        return false;
    }
}

void SpeedSensorInterrupt::signal()
{
    counter++;
}

void SpeedSensorInterrupt::setup()
{
    #ifndef NATIVE
    if (pin >= 0 && n >= 0 && n < MAX_INSTANCES)
    {
        instances[n] = this;
        pinMode(pin, INPUT);
        attachInterrupt(digitalPinToInterrupt(pin), signal_wrappers[n], RISING);
        Log::tracex("SPEED_SENSOR_INTERRUPT", "Setup", "Attached interrupt on pin {%d} for instance {%d}", pin, n);
    }            
    #endif
}
