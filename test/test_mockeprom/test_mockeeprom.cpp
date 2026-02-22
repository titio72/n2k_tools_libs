#include "MockEEPROM.h"
#include <unity.h>

void test_mockeprom_init() {
    MockEEPROM eeprom;
    eeprom.begin(512);
    TEST_ASSERT_EQUAL(512, eeprom.size());
}

struct TestData {
    uint8_t a;
    uint16_t b;
    char c[128];
};

void test_mockeprom_write_read_struct() {
    MockEEPROM eeprom;
    eeprom.begin(512);

    TestData data = {0xAB, 0x1234, "Hello, EEPROM!"};
    eeprom.writeBytes(20, &data, sizeof(TestData));

    TestData read_data;
    eeprom.readBytes(20, &read_data, sizeof(TestData));

    TEST_ASSERT_EQUAL(data.a, read_data.a);
    TEST_ASSERT_EQUAL(data.b, read_data.b);
    TEST_ASSERT_EQUAL_STRING(data.c, read_data.c);
}

void test_mockeprom_write_read() {
    MockEEPROM eeprom;
    eeprom.begin(512);

    uint8_t value = 0xAB;
    eeprom.write(10, value);
    TEST_ASSERT_EQUAL(value, eeprom.read(10));
}
    
int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_mockeprom_init);
    RUN_TEST(test_mockeprom_write_read);
    RUN_TEST(test_mockeprom_write_read_struct);
    UNITY_END();
    return 0;
}
