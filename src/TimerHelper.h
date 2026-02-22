#ifndef _N2K_UTILS_TIMERHELPER_HPP
#define _N2K_UTILS_TIMERHELPER_HPP

class SpeedSensor;

void add_tacho(SpeedSensor* tachometer);
void remove_tacho(SpeedSensor* tachometer);
bool contains_tacho(const SpeedSensor* tachometer);
void init_timer();

#endif //_N2K_UTILS_TIMERHELPER_HPP