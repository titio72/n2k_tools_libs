#ifndef N2K_H
#define N2K_H

#include <N2kMessages.h>
#include "Utils.h"
#include <vector>
#include <string>

#ifndef N2K_SOURCE_DEFAULT
#define N2K_SOURCE_DEFAULT  22
#endif

class tNMEA2000;
class tN2kDeviceList;

class N2KStats
{
public:
    unsigned long recv = 0;
    unsigned long sent = 0;
    unsigned long fail = 0;
    unsigned char canbus = 0;
    void dump();
    void reset();
    void dump_and_reset();
};

struct n2k_device_info
{
    std::string ModelSerialCode = "0.0.1";
    unsigned short ProductCode = 100;
    std::string ModelID = "AB";
    std::string SwCode = "AB 0.0.1";
    std::string ModelVersion = "0001";

    unsigned long UniqueNumber = 1;     // Unique number. Use e.g. Serial number.
    unsigned char DeviceFunction = 145; // Device function=Analog to NMEA 2000 Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
    unsigned char DeviceClass = 60;     // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
    uint16_t ManufacturerCode = 2046;   // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
    unsigned char LoadEquivalency = 1;  // bus load, x50 mA
};

typedef void (*n2k_msg_handler)(const tN2kMsg &N2kMsg);
typedef void (*n2k_source_change_handler)(const unsigned char old_source, const unsigned char new_source);
typedef void (*n2k_sent_message_handler)(const tN2kMsg &N2kMsg, bool success);

struct n2k_device_summary
{
    unsigned char source;
    uint16_t manufacturer_code;
    const char* model_id;       // may be NULL
    const char* model_version;  // may be NULL
};
typedef bool (*n2k_device_matcher)(const n2k_device_summary &device);

class N2K {

    public:
        static N2K* get_instance(n2k_msg_handler _msg_handler, n2k_source_change_handler _src_handler);

        virtual ~N2K();

        void setup(n2k_device_info dvc);

        void loop(unsigned long time);

        bool send_msg(const tN2kMsg &N2kMsg);

        bool is_initialized();

        bool is_bus_connected();

        // used only on linux
        void set_can_socket_name(const char* name);

        N2KStats getStats();

        unsigned char get_source();
        void set_desired_source(unsigned char src);

        void add_pgn(unsigned long pgns);

        /** true: also receive messages addressed to other devices (N2km_ListenAndNode). Default false (NodeOnly). Before setup(). */
        void set_listen_all(bool listen_all);

        /** PGNs advertised as received. Before setup(). */
        void add_rx_pgn(unsigned long pgn);

        /** Track the devices on the bus (tN2kDeviceList). Before setup(). */
        void enable_device_list();

        /** First device accepted by matcher: stores its source address and returns true. False if there is no device list or no match. */
        bool find_device(n2k_device_matcher matcher, unsigned char &source);

        static void set_sent_message_callback(n2k_sent_message_handler _MsgHandler);

    private:
        N2K();
        tNMEA2000* NMEA2000;
        char socket_name[32];
        unsigned char desired_source;
        std::vector<unsigned long> pgns;
        n2k_device_info device_info;
        bool listen_all = false;
        bool want_device_list = false;
        tN2kDeviceList* device_list = nullptr;
        std::vector<unsigned long> rx_pgns;
        // The NMEA2000 library keeps the pointers it is given, so these must outlive setup()
        std::vector<unsigned long> tx_list;
        std::vector<unsigned long> rx_list;

};

#endif
