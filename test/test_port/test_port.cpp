#include "Ports.h"
#include "MockPort.hpp"
#include <unity.h>
#include <string>
#include <vector>

// Captures callbacks from Port so tests can assert on what was delivered.
class TestListener : public PortListener
{
public:
    std::vector<std::string> lines;
    std::vector<std::string> partials;
    int partial_x_calls = 0;

    void on_line_read(const char *line) override
    {
        lines.push_back(std::string(line));
    }

    void on_partial(const char *line) override
    {
        partials.push_back(std::string(line));
    }

    void on_partial_x(const char *line, int len) override
    {
        partial_x_calls++;
    }
};

// ---------- open / close ----------

void test_port_initial_state_is_closed()
{
    MockPort port;
    TEST_ASSERT_FALSE(port.is_open());
    TEST_ASSERT_EQUAL(0, port.get_open_count());
}

void test_port_open_sets_open_state()
{
    MockPort port;
    int result = port.open();
    TEST_ASSERT_TRUE(port.is_open());
    TEST_ASSERT_EQUAL(1, port.get_open_count());
    TEST_ASSERT_EQUAL(1, result);
}

void test_port_close_sets_closed_state()
{
    MockPort port;
    port.open();
    port.close();
    TEST_ASSERT_FALSE(port.is_open());
    TEST_ASSERT_EQUAL(1, port.get_close_count());
}

// ---------- listen() / line assembly ----------

void test_port_listen_reads_single_line()
{
    MockPort port;
    TestListener listener;
    port.set_handler(&listener);
    port.open();

    port.simulate_line("Hello");
    port.listen(100);

    TEST_ASSERT_EQUAL(1, listener.lines.size());
    TEST_ASSERT_EQUAL_STRING("Hello", listener.lines[0].c_str());
}

void test_port_listen_reads_multiple_lines()
{
    MockPort port;
    TestListener listener;
    port.set_handler(&listener);
    port.open();

    const char *rawlines[] = {"AAA", "BBB"};
    port.simulate_lines(rawlines, 2);
    port.listen(100);

    TEST_ASSERT_EQUAL(2, listener.lines.size());
    TEST_ASSERT_EQUAL_STRING("AAA", listener.lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("BBB", listener.lines[1].c_str());
}

void test_port_crlf_produces_single_line_callback()
{
    MockPort port;
    TestListener listener;
    port.set_handler(&listener);
    port.open();

    port.simulate_data("XYZ\r\n");
    port.listen(100);

    // The \r closes the line; the following \n must not trigger a second callback.
    TEST_ASSERT_EQUAL(1, listener.lines.size());
    TEST_ASSERT_EQUAL_STRING("XYZ", listener.lines[0].c_str());
}

void test_port_partial_data_without_newline_triggers_on_partial()
{
    MockPort port;
    TestListener listener;
    port.set_handler(&listener);
    port.open();

    port.simulate_data("AB");
    port.listen(100);

    TEST_ASSERT_EQUAL(0, listener.lines.size());
    TEST_ASSERT_EQUAL(2, listener.partials.size());
    TEST_ASSERT_EQUAL_STRING("A", listener.partials[0].c_str());
    TEST_ASSERT_EQUAL_STRING("AB", listener.partials[1].c_str());
    TEST_ASSERT_EQUAL(2, listener.partial_x_calls);
}

// ---------- error / no-data handling ----------

void test_port_read_error_closes_port()
{
    MockPort port;
    port.open();
    port.set_error_on_read(true);

    port.listen(50);

    TEST_ASSERT_FALSE(port.is_open());
    TEST_ASSERT_EQUAL(1, port.get_close_count());
}

void test_port_nothing_to_read_returns_without_closing()
{
    MockPort port;
    port.open();

    port.listen(10); // empty queue: should return immediately, port stays open

    TEST_ASSERT_TRUE(port.is_open());
    TEST_ASSERT_EQUAL(0, port.get_close_count());
}

// ---------- auto (re)open behavior ----------

void test_port_auto_opens_on_first_listen_when_closed()
{
    MockPort port; // never explicitly opened

    TEST_ASSERT_FALSE(port.is_open());
    port.listen(10);

    TEST_ASSERT_TRUE(port.is_open());
    TEST_ASSERT_EQUAL(1, port.get_open_count());
}

void test_port_speed_change_forces_reopen()
{
    MockPort port;
    port.open(); // open_count=1, last_open_try untouched (still 0)

    port.set_speed(9600); // differs from the default speed used at open()
    port.listen(50);

    // listen() must close the port because the speed changed, then
    // immediately reopen it since the open-retry throttle was never armed.
    TEST_ASSERT_EQUAL(1, port.get_close_count());
    TEST_ASSERT_EQUAL(2, port.get_open_count());
    TEST_ASSERT_TRUE(port.is_open());
}

void test_port_reopen_is_throttled_within_one_second()
{
    MockPort port; // closed

    port.listen(10); // auto-opens, arming the retry throttle
    TEST_ASSERT_EQUAL(1, port.get_open_count());

    port.close();
    port.listen(10); // called immediately after: throttle should block reopen

    TEST_ASSERT_FALSE(port.is_open());
    TEST_ASSERT_EQUAL(1, port.get_open_count());
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_port_initial_state_is_closed);
    RUN_TEST(test_port_open_sets_open_state);
    RUN_TEST(test_port_close_sets_closed_state);

    RUN_TEST(test_port_listen_reads_single_line);
    RUN_TEST(test_port_listen_reads_multiple_lines);
    RUN_TEST(test_port_crlf_produces_single_line_callback);
    RUN_TEST(test_port_partial_data_without_newline_triggers_on_partial);

    RUN_TEST(test_port_read_error_closes_port);
    RUN_TEST(test_port_nothing_to_read_returns_without_closing);

    RUN_TEST(test_port_auto_opens_on_first_listen_when_closed);
    RUN_TEST(test_port_speed_change_forces_reopen);
    RUN_TEST(test_port_reopen_is_throttled_within_one_second);

    UNITY_END();
    return 0;
}
