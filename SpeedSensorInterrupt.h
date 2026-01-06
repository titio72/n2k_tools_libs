#ifndef _SPEED_SENSOR_INTERRUPT_H
#define _SPEED_SENSOR_INTERRUPT_H

#include <stdint.h>

class SpeedSensorInterrupt
{
public:
    SpeedSensorInterrupt(int pin, int n = -1);
    ~SpeedSensorInterrupt();

    unsigned long get_sample_age() const { return last_read_time; }

    void setup();

    bool read_data(unsigned long milliseconds, double &frequency, int &counter_out);

    void set_alpha(double a) { alpha = a; }
    double get_alpha() const { return alpha; }

    int get_pin() const { return pin; }

    // used for tests
    void signal();

private:
    unsigned long last_read_time = 0;

    unsigned long counter = 0;
    double smooth_counter = 0.0;

    double alpha = 1.0;

    int pin;
    int n;
};

#endif