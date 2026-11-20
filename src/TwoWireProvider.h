#ifndef _TWOWIRE_PROVIDER_H
#define _TWOWIRE_PROVIDER_H

#include <Wire.h>

class TwoWire;

class TwoWireProvider
{
public:
    static TwoWire* get_two_wire();
    static inline uint32_t clock = 100000; // Default I2C clock 100kHz
private:
    TwoWireProvider();
};

#endif
