
#include <Log.h>
#include "BTInterface.h"
#include <string>
#include <stdint.h>
#include <Utils.h>

#ifndef NATIVE
#include <Arduino.h>
#include <NimBLEDevice.h>

class InternalBLEStateImpl
    : public InternalBLEState,
      public NimBLECharacteristicCallbacks,
      public NimBLEServerCallbacks
{
private:
    NimBLEServer *pServer = nullptr;
    NimBLEService *pService = nullptr;
    std::vector<NimBLECharacteristic *> characteristicsSettings;
    std::vector<NimBLECharacteristic *> characteristicsFields;
    ABBLEWriteCallback *clientWriteCallback = nullptr;
    std::string name = "";
    std::string uuid = "";
    uint32_t passkey = 0;
    uint16_t conn_min = 400, conn_max = 400, conn_latency = 4, conn_timeout = 500;
    std::vector<bool> fieldNotify;

public:
    InternalBLEStateImpl() {}
    ~InternalBLEStateImpl() {}

    void init(const char *n, const char *u, ABBLEWriteCallback *c)
    {
        name = n;
        uuid = u;
        clientWriteCallback = c;
    }

    void set_passkey(uint32_t pk) override
    {
        passkey = pk;
    }

    void change_passkey(uint32_t pk) override
    {
        passkey = pk;
        if (passkey != 0)
            NimBLEDevice::setSecurityPasskey(passkey);
    }

    void set_conn_params(uint16_t min_interval, uint16_t max_interval, uint16_t latency, uint16_t timeout) override
    {
        conn_min = min_interval;
        conn_max = max_interval;
        conn_latency = latency;
        conn_timeout = timeout;
    }

    // NimBLECharacteristicCallbacks
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
    {
        if (clientWriteCallback == nullptr) return;

        auto val = pCharacteristic->getValue();

        int i = 0;
        for (i = 0; i < (int)characteristicsSettings.size(); i++)
        {
            if (characteristicsSettings[i]->getUUID() == pCharacteristic->getUUID())
                break;
        }
        if (i < (int)characteristicsSettings.size())
            clientWriteCallback->on_write_bytes(i, (const uint8_t *)val.data(), val.size());
    }

    // NimBLEServerCallbacks
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo)
    {
        Log::trace("[BLE] Connected to client\n");
        // Defaults: 500 ms CI (400 x 1.25 ms), slave latency 4, supervision 5 s. See set_conn_params().
        pServer->updateConnParams(connInfo.getConnHandle(), conn_min, conn_max, conn_latency, conn_timeout);
        NimBLEDevice::getAdvertising()->start();
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason)
    {
        Log::trace("[BLE] Disconnected from client\n");
    }

    void setup(const std::vector<ABBLEField> &fields, const std::vector<ABBLESetting> &settings)
    {
        Log::tracex("BLE", "Setup", "device {%s}", name.c_str());
        NimBLEDevice::init(name);
        NimBLEDevice::setMTU(128);
        NimBLEDevice::setPower(ESP_PWR_LVL_N0); // 0 dBm, sufficient for cabin range
        pServer = NimBLEDevice::createServer();
        pServer->setCallbacks(this);
        pService = pServer->createService(uuid.c_str());
        for (int i = 0; i < (int)settings.size(); i++)
        {
            const ABBLESetting &s = settings.at(i);
            // With a passkey, writes need an encrypted + authenticated (MITM) link
            uint32_t props = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE;
            if (passkey != 0 && s.secured)
                props |= NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN;
            if (passkey != 0 && s.secured_read)
                props |= NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN;
            NimBLECharacteristic *c = pService->createCharacteristic(s.c_uuid.c_str(), props);
            c->setCallbacks(this);
            characteristicsSettings.push_back(c);
            Log::tracex("BLE", "Setting", "UUID {%s}", s.c_uuid.c_str());
        }
        fieldNotify.clear();
        for (int i = 0; i < (int)fields.size(); i++)
        {
            const ABBLEField &s = fields.at(i);
            // NimBLE adds the CCCD descriptor for INDICATE/NOTIFY automatically
            uint32_t props = NIMBLE_PROPERTY::READ | (s.notify ? NIMBLE_PROPERTY::NOTIFY : NIMBLE_PROPERTY::INDICATE);
            if (passkey != 0 && s.secured_read)
                props |= NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN;
            NimBLECharacteristic *cc = pService->createCharacteristic(s.c_uuid.c_str(), props);
            characteristicsFields.push_back(cc);
            fieldNotify.push_back(s.notify);
            Log::tracex("BLE", "Field", "UUID {%s}", s.c_uuid.c_str());
        }
        Log::tracex("BLE", "Loaded", "Settings {%d} Fields {%d}", settings.size(), fields.size());
    }

    void end()
    {
        NimBLEDevice::deinit(false); // false = keep bonding data in NVS
    }

    void begin()
    {
        Log::tracex("BLE", "Starting BLE", "device {%s}", name.c_str());
        pService->start();

        if (passkey != 0)
        {
            // Bonding + MITM + Secure Connections, static passkey entered on the peer
            NimBLEDevice::setSecurityAuth(true, true, true);
            NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
            NimBLEDevice::setSecurityPasskey(passkey);
        }
        else
        {
            // Just Works bonding with Secure Connections, no MITM
            NimBLEDevice::setSecurityAuth(true, false, true);
            NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
        }

        // 1000 ms advertising interval (1600 x 0.625 ms) vs ~100 ms default
        NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
        pAdv->setMinInterval(1600);
        pAdv->setMaxInterval(1600);
        pAdv->addServiceUUID(uuid.c_str());
        // The 128-bit service UUID already fills most of the 31-byte legacy adv
        // packet, leaving no room for the name there - put it in the scan response instead.
        pAdv->enableScanResponse(true);
        pAdv->setName(name);
        pAdv->start();
    }

    void publish(int handle)
    {
        if (fieldNotify[handle])
            characteristicsFields[handle]->notify();
        else
            characteristicsFields[handle]->indicate();
    }

    void set_field_value(int handle, const char *value)
    {
        if (handle >= 0 && handle < (int)characteristicsFields.size())
        {
            characteristicsFields[handle]->setValue(value);
            publish(handle);
        }
    }

    void set_field_value(int handle, uint16_t value)
    {
        if (handle >= 0 && handle < (int)characteristicsFields.size())
        {
            characteristicsFields[handle]->setValue(value);
            publish(handle);
        }
    }

    void set_field_value(int handle, void *value, int len)
    {
        if (handle >= 0 && handle < (int)characteristicsFields.size())
        {
            characteristicsFields[handle]->setValue((uint8_t *)value, len);
            publish(handle);
        }
    }

    ByteBuffer get_field_value(int handle)
    {
        if (handle >= 0 && handle < (int)characteristicsFields.size())
        {
            auto val = characteristicsFields[handle]->getValue();
            return ByteBuffer((uint8_t *)val.data(), val.size());
        }
        return ByteBuffer(0);
    }

    void set_setting_value(int handle, const char *value)
    {
        if (handle >= 0 && handle < (int)characteristicsSettings.size())
            characteristicsSettings[handle]->setValue(value);
    }

    void set_setting_value(int handle, int value)
    {
        if (handle >= 0 && handle < (int)characteristicsSettings.size())
        {
            static char temp[16];
            itoa(value, temp, 10);
            characteristicsSettings[handle]->setValue(temp);
        }
    }

    void change_device_name(const char *n)
    {
        name = std::string(n).substr(0, 15);
        if (pServer)
        {
            NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
            pAdv->stop();
            NimBLEDevice::setDeviceName(name);
            pAdv->setName(name);
            pAdv->start();
            Log::tracex("BLE", "Change device name", "name {%s}", name.c_str());
        }
    }

    const char *get_device_name()
    {
        return name.c_str();
    }
};
#define USE_REAL_BLE_IMPLEMENTATION
#endif

BTInterface::BTInterface(const char *uuid, const char *name, ABBLEWriteCallback *cmd_cback, InternalBLEState *internalState) : init(false)
{
    internalStateOwned = false;
#ifdef USE_REAL_BLE_IMPLEMENTATION
    if (internalState == nullptr)
    {
        internalStateOwned = true;
        internalState = new InternalBLEStateImpl();
    }
#endif
    state = internalState;
    if (state)
    {
        state->init(name, uuid, cmd_cback);
    }
}

BTInterface::~BTInterface()
{
    if (state)
    {
        state->end();
#ifdef USE_REAL_BLE_IMPLEMENTATION
        if (internalStateOwned)
            delete state;
#endif
        state = nullptr;
    }
}

int BTInterface::add_setting(const char *name, const char *uuid, bool secured, bool secured_read)
{
    ABBLESetting s(name, uuid, secured, secured_read);
    settings.push_back(s);
    return settings.size() - 1;
}

int BTInterface::add_field(const char *name, const char *uuid, bool notify, bool secured_read)
{
    ABBLEField f(name, uuid, notify, secured_read);
    fields.push_back(f);
    return fields.size() - 1;
}

void BTInterface::set_field_value(int handle, const char *value)
{
    if (state)
        state->set_field_value(handle, value);
}

void BTInterface::set_field_value(int handle, uint16_t value)
{
    if (state)
        state->set_field_value(handle, value);
}

void BTInterface::set_field_value(int handle, void *value, int len)
{
    if (state)
        state->set_field_value(handle, value, len);
}

void BTInterface::set_setting_value(int handle, const char *value)
{
    if (state)
        state->set_setting_value(handle, value);
}

void BTInterface::set_setting_value(int handle, int value)
{
    if (state)
        state->set_setting_value(handle, value);
}

ByteBuffer BTInterface::get_field_value(int handle)
{
    if (state)
        return state->get_field_value(handle);
    return ByteBuffer(0);
}

void BTInterface::setup()
{
    if (!init)
    {
        init = true;
        if (state)
            state->setup(fields, settings);
    }
}

void BTInterface::begin()
{
    if (init && state)
        state->begin();
}

void BTInterface::loop(unsigned long milli_seconds)
{
}

void BTInterface::set_device_name(const char *name)
{
    if (state)
        state->change_device_name(name);
}

void BTInterface::set_passkey(uint32_t passkey)
{
    if (state)
        state->set_passkey(passkey);
}

void BTInterface::change_passkey(uint32_t passkey)
{
    if (state)
        state->change_passkey(passkey);
}

void BTInterface::set_conn_params(uint16_t min_interval, uint16_t max_interval, uint16_t latency, uint16_t timeout)
{
    if (state)
        state->set_conn_params(min_interval, max_interval, latency, timeout);
}

const char *BTInterface::get_device_name()
{
    if (state)
        return state->get_device_name();
    return nullptr;
}
