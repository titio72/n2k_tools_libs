
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

public:
    InternalBLEStateImpl() {}
    ~InternalBLEStateImpl() {}

    void init(const char *n, const char *u, ABBLEWriteCallback *c)
    {
        name = n;
        uuid = u;
        clientWriteCallback = c;
    }

    // NimBLECharacteristicCallbacks
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo)
    {
        if (clientWriteCallback == nullptr) return;

        static char v[256];
        strncpy(v, pCharacteristic->getValue().c_str(), sizeof(v) - 1);
        v[sizeof(v) - 1] = '\0';

        int i = 0;
        for (i = 0; i < (int)characteristicsSettings.size(); i++)
        {
            if (characteristicsSettings[i]->getUUID() == pCharacteristic->getUUID())
                break;
        }
        if (i < (int)characteristicsSettings.size())
            clientWriteCallback->on_write(i, v);
    }

    // NimBLEServerCallbacks
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo)
    {
        Log::trace("[BLE] Connected to client\n");
        // Request 500 ms CI (400 x 1.25 ms), slave latency 4, supervision 5 s
        pServer->updateConnParams(connInfo.getConnHandle(), 400, 400, 4, 500);
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
            NimBLECharacteristic *c = pService->createCharacteristic(
                s.c_uuid.c_str(), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
            c->setCallbacks(this);
            characteristicsSettings.push_back(c);
            Log::tracex("BLE", "Setting", "UUID {%s}", s.c_uuid.c_str());
        }
        for (int i = 0; i < (int)fields.size(); i++)
        {
            const ABBLEField &s = fields.at(i);
            // NimBLE adds the CCCD descriptor for INDICATE automatically
            NimBLECharacteristic *c = pService->createCharacteristic(
                s.c_uuid.c_str(), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE);
            characteristicsFields.push_back(c);
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

        // Just Works bonding with Secure Connections, no MITM
        NimBLEDevice::setSecurityAuth(true, false, true);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

        // 1000 ms advertising interval (1600 x 0.625 ms) vs ~100 ms default
        NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
        pAdv->setMinInterval(1600);
        pAdv->setMaxInterval(1600);
        pAdv->addServiceUUID(uuid.c_str());
        pAdv->setName(name);
        pAdv->start();
    }

    void set_field_value(int handle, const char *value)
    {
        if (handle >= 0 && handle < (int)characteristicsFields.size())
        {
            characteristicsFields[handle]->setValue(value);
            characteristicsFields[handle]->indicate();
        }
    }

    void set_field_value(int handle, uint16_t value)
    {
        if (handle >= 0 && handle < (int)characteristicsFields.size())
        {
            characteristicsFields[handle]->setValue(value);
            characteristicsFields[handle]->indicate();
        }
    }

    void set_field_value(int handle, void *value, int len)
    {
        if (handle >= 0 && handle < (int)characteristicsFields.size())
        {
            characteristicsFields[handle]->setValue((uint8_t *)value, len);
            characteristicsFields[handle]->indicate();
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

int BTInterface::add_setting(const char *name, const char *uuid)
{
    ABBLESetting s(name, uuid);
    settings.push_back(s);
    return settings.size() - 1;
}

int BTInterface::add_field(const char *name, const char *uuid)
{
    ABBLEField f(name, uuid);
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

const char *BTInterface::get_device_name()
{
    if (state)
        return state->get_device_name();
    return nullptr;
}
