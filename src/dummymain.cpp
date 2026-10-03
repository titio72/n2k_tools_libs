// Only used when the library is built on its own (pio run -e esp32-c3) so that the real
// NimBLE and NMEA2000 code can be compile-checked. Never defined when used as a dependency.
#ifdef LIB_STANDALONE_BUILD
#include <Arduino.h>
void setup() {}
void loop() {}
#endif
