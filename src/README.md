# n2k_tools_libs
General purpose tools for nmea2000 applications

## Agents

`Agents.hpp` provides the agent pattern: a class implements `setup/enable/disable/is_enabled/loop` (macro `AB_AGENT`,
the application declares a `Context` type first) and `handle_agent_loop()` drives it with enable/disable from a
runtime flag, retries and loop timing.

## BLE (`BTInterface`)

- `ABBLEWriteCallback::on_write_bytes(handle, data, len)`: raw bytes of a write (binary payloads). The default
  falls back to `on_write()` with a NUL-terminated string.
- `add_setting(name, uuid, secured = true, secured_read = false)`, `add_field(name, uuid, notify = false, secured_read = false)`:
  with a passkey set, `secured` protects writes and `secured_read` protects reads. `notify` selects notifications
  instead of indications.
- `set_passkey()` before `setup()`, `change_passkey()` at any time afterwards (a pairing window can be built by
  switching between the real passkey and a random one).
- `set_conn_params(min, max, latency, timeout)` before `begin()` (units 1.25 ms, 1.25 ms, events, 10 ms).

## N2K

- `set_listen_all(true)` before `setup()`: `N2km_ListenAndNode` instead of `N2km_NodeOnly`.
- `add_rx_pgn(pgn)` / `add_pgn(pgn)`: advertised receive / transmit PGNs.
- `enable_device_list()` and `find_device(matcher, source)`: look up a device on the bus (for instance by model id).
- `n2k_device_info::LoadEquivalency`: bus load in units of 50 mA.
