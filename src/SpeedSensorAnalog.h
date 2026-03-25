#ifndef _SPEED_SENSOR_ANALOG_H
#define _SPEED_SENSOR_ANALOG_H

#include <stdint.h>

class SpeedSensorAnalog
{
public:
    SpeedSensorAnalog(int pin);
    ~SpeedSensorAnalog();

    unsigned long get_sample_age() const { return last_read_time; }

    void setup();

    int get_counter() const { return counter; }

    bool read_data(unsigned long milliseconds, double &frequency, int &counter_out);

    void loop_micros(unsigned long now_micros);

    void set_alpha(double a) { alpha = a; }
    double get_alpha() const { return alpha; }

    int get_pin() const { return pin; }

    // used for tests
    void read_signal(int state, unsigned long t_micros = 0);

    uint16_t max = 0x000;   // use for calibration, not used in calculations
    uint16_t min = 0xFFFF;  // use for calibration, not used in calculations

    uint16_t high_threshold = 3600; // ~2.8V for a 10-bit ADC with 3.3V reference, used to determine if the signal is HIGH
    uint16_t low_threshold =   600; // ~0.5V for a 10-bit ADC with 3.3V reference, used to determine if the signal is LOW

private:
    unsigned long last_read_time = 0;
    unsigned long counter = 0;
    double smooth_counter = 0.0;
    int state = LOW;
    double alpha = 1.0;
    int pin;
};

#endif
