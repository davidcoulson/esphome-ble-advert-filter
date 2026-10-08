# Options

Everything is optional. An option you leave out does nothing, so you only pay for what you
turn on.

- [Filter options](#filter-options)
- [Apple: HomeKit, FindMy and iBeacons](#apple-homekit-findmy-and-ibeacons)
- [Sensors](#sensors)
- [Threshold dial (number)](#threshold-dial-number)
- [Actions](#actions)
- [Using it from lambdas](#using-it-from-lambdas)

## Filter options

```yaml
ble_advert_filter:
  id: my_filter                 # only needed for lambdas
  bluetooth_proxy_id: ...       # only if you have more than one bluetooth_proxy
```

### Distance

RSSI is signal strength in dBm. It's always negative: -50 is close, -90 is far. Each 6 dB is
roughly double the distance, but walls and bodies make it a rough guide at best.

| Option | Default | What it does |
|---|---|---|
| `rssi_threshold` | off | Drop anything weaker than this. Your own devices (below) can have their own limits instead. |
| `rssi_floor` | off | Drop anything weaker than this, including your own devices. A backstop for signals too weak to be useful for anything. |
| `rssi_mac_allowlist` | `rssi_floor` | Limit for devices on `mac_allowlist`. By default they ignore `rssi_threshold` and only the floor applies. |
| `rssi_irk` | `rssi_threshold` | Limit for your phones and watches (matched by `irks`). |
| `rssi_service_uuid` | `rssi_threshold` | Limit for devices matched by `service_uuid_allowlist`. |

Values go from -127 to 0; -127 means off. The config check stops two mistakes:

- `rssi_floor` higher than `rssi_threshold`, because the floor would then override the
  threshold for everything;
- a per-device limit below `rssi_floor`, because the floor would drop those adverts first and
  the limit would never do anything.

### Your devices

| Option | Default | What it does |
|---|---|---|
| `irks` | none | IRKs of your phones and watches. With any set, an advert from a rotating private address that matches none of them is dropped as someone else's. See [Getting your IRKs](irks.md). |
| `mac_allowlist` | none | Addresses that always get through, whatever else you've blocked (except `mac_blocklist`). Only `rssi_floor` and `rssi_mac_allowlist` apply. |
| `allowlist_exclusive` | `false` | Turn `mac_allowlist` into the only way through: everything not on it is dropped. |
| `service_uuid_allowlist` | none | Service UUIDs that always get through: 16-bit (`0xFFF6`) or full 128-bit. Use it to keep pairing working; see below. |
| `allow_espressif` | `true` | Never treat an Espressif (ESP32) address as someone else's phone. |

Devices in pairing or setup mode often advertise from a random address that changes, so you
can't put them in `mac_allowlist` ahead of time, and with `irks` or `drop_non_resolvable` on
they'd be dropped. Listing the service UUID they advertise keeps them visible:

```yaml
  service_uuid_allowlist:
    - 0xFFF6                                   # Matter commissioning
    - "00467768-6228-2272-4663-277478268000"   # Improv Wi-Fi
```

### Blocking

| Option | Default | What it does |
|---|---|---|
| `mac_blocklist` | none | Addresses this proxy ignores completely. Wins over every allow option. |
| `manufacturer_blocklist` | none | Bluetooth company IDs to drop, for example `0x004C` for Apple. Your own devices are never dropped by this. |
| `name_blocklist` | none | Drop adverts whose name contains any of these words. Not case-sensitive, up to 29 characters each. |
| `drop_non_resolvable` | `false` | Drop adverts from "non-resolvable" random addresses. These change constantly and carry no identity, so nothing, including Home Assistant, can ever track them. |

`mac_blocklist` is handy when several proxies can hear a device that needs a single, stable
connection, such as some smart locks. Blocking it on the proxies you don't want handling it
stops them competing for the connection, without making any proxy dedicated to it.

A company ID list is at the Bluetooth SIG's
[assigned numbers](https://www.bluetooth.com/specifications/assigned-numbers/) page.

## Apple: HomeKit, FindMy and iBeacons

Blocking Apple (`0x004C`) removes a lot of noise, but some things you might want also use
Apple's ID. These options let them back in.

| Option | Default | What it does |
|---|---|---|
| `allow_homekit` | `true` | HomeKit accessories are never blocked as Apple noise. |
| `allow_findmy` | `false` | Let AirTags, AirPods and other Find My items through. `true`, or `rssi: -85` to give them their own distance limit. |
| `allow_ibeacon` | `false` | Let iBeacons through. `true` for all of them, or a list of rules to allow only yours. |

Your own iPhones and Apple Watches don't need any of these: if you've added their IRKs, they
get through anyway.

### FindMy

```yaml
  allow_findmy: true      # uses rssi_threshold
  # or
  allow_findmy:
    rssi: -85             # its own limit, replacing rssi_threshold and rssi_floor
```

There's no way to pick out only your AirTags: their adverts don't say whose they are. Every
FindMy item in range gets through, so give them a sensible `rssi` if your neighbours have
lots.

### iBeacons

`true` lets every iBeacon through. Usually you only want your own, which you pick by UUID (and
optionally major and minor):

```yaml
  allow_ibeacon:
    - uuid: fde3b150-2f64-43ba-aee9-867f75ee4a6f   # all of mine
    - uuid: 12345678-1234-1234-1234-123456789abc
      major: 1
      minor: [3, 4]                                # just these two
      rssi: -95                                    # with their own distance limit
```

- Each rule needs a `uuid`, a `major`, or both. `minor` needs a `major`.
- Always include the `uuid` if you can. Many beacons ship with `major: 1`, so a rule with only
  `major: 1` can let in other people's beacons left on factory settings.
- `rssi` on a rule replaces both `rssi_threshold` and `rssi_floor` for the beacons it matches.
  Leave it out to use the normal limits.
- If several rules match, the most specific one wins.

## Sensors

```yaml
sensor:
  - platform: ble_advert_filter
    update_interval: 60s
    forwarded:
      name: "BLE Adverts Forwarded"
```

All keys are optional and take normal [sensor options](https://esphome.io/components/sensor/).

| Key | Unit | What it shows |
|---|---|---|
| `forwarded` | adv/min | Adverts sent to Home Assistant |
| `dropped` | adv/min | Adverts dropped, for any reason |
| `drop_rate` | % | Share of adverts dropped. Shows unknown, not 0, when nothing was heard |
| `dropped_rpa` | adv/min | Dropped as someone else's phone or watch (needs `irks`) |
| `forwarded_irk` | adv/min | Let through because they matched one of your IRKs |
| `allowed_service_uuid` | adv/min | Let through only because of `service_uuid_allowlist`. Stays at 0 unless something is pairing |
| `dropped_floor` | adv/min | Dropped by `rssi_floor`. If this climbs, the floor may be cutting off devices you track |
| `dropped_gate` | adv/min | Dropped for being weaker than any of your limits allow |
| `irk_count` | | IRKs currently loaded |

Rates are worked out from the real time between updates, so they're correct for any
`update_interval`.

## Threshold dial (number)

```yaml
number:
  - platform: ble_advert_filter
    rssi_threshold:
      name: "BLE RSSI Threshold"
      min_value: -90        # default: rssi_floor, or -100 if you have no floor
      max_value: -30        # default
      restore_value: true   # default
```

Puts `rssi_threshold` on a dial in Home Assistant so you can tune each proxy without
reflashing. The YAML value is the starting point. With `restore_value`, whatever you set in
Home Assistant is kept across reboots and used from then on.

`min_value` can't be lower than `rssi_floor`: the floor already drops anything weaker, so those
settings would do nothing. Set `rssi_threshold` in the YAML too, or the dial starts out off
(-127), below its own range.

## Actions

```yaml
on_...:
  - ble_advert_filter.set_irks: !lambda "return x;"
  - ble_advert_filter.clear_irks:
```

- `ble_advert_filter.set_irks`: replace the IRK list. Takes text containing the keys, or
  `irks:` with a value or lambda. Text with no valid key is ignored and the current list kept.
  See [Loading IRKs from Home Assistant](irks.md#loading-irks-from-home-assistant).
- `ble_advert_filter.clear_irks`: empty the list on purpose. This turns IRK checking off.

Both take an optional `id` if you have more than one filter.

## Using it from lambdas

Give the filter an `id` and call these from any lambda:

| Method | What it does |
|---|---|
| `set_rssi_threshold(int8_t)`, `get_rssi_threshold()` | Change or read the threshold |
| `set_irks(std::string)` | Same as the action. Returns how many keys loaded, or -1 if none were found |
| `clear_irks()` | Same as the action |
| `get_irk_count()`, `get_irks()` | The loaded keys |
| `get_adv_forwarded()`, `get_adv_dropped()`, `get_adv_dropped_rpa()`, `get_adv_forwarded_irk()`, `get_adv_allowed_service_uuid()`, `get_adv_dropped_floor()`, `get_adv_dropped_gate()` | Running totals since boot, the numbers behind the sensors |

The other runtime setters in
[`ble_advert_filter.h`](../components/ble_advert_filter/ble_advert_filter.h) work too.
