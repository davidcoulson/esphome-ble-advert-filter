# esphome-ble-advert-filter

On-device BLE advertisement filtering for ESPHome's `bluetooth_proxy`.

A proxy forwards every advertisement it hears. In a house with many BLE devices — or many
proxies — most of that is untrackable noise: passing phones and watches rotating their
addresses, and Apple Continuity traffic no integration consumes. This drops that traffic on
the device, before it crosses the network.

Measured on one ESP32-S3 Ethernet proxy in a 60+ proxy deployment: **6,197 adv/min dropped
against 3,991 forwarded — a 60.7% reduction**, of which 2,660 were unresolvable RPAs.

## Requires the upstream filter hook

This component installs itself into `bluetooth_proxy`'s `AdvertisementFilter` slot, added in
[esphome/esphome#19220](https://github.com/esphome/esphome/pull/19220). **It will not compile
against an ESPHome release that predates that hook.**

This replaces an earlier approach that vendored a whole fork of `bluetooth_proxy`, which had
to be re-synced on every ESPHome release. Against the hook there is nothing to re-sync.

## Usage

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/davidcoulson/esphome-ble-advert-filter
    components: [ble_advert_filter]

bluetooth_proxy:
  active: true

ble_advert_filter:
  rssi_threshold: -85
  drop_non_resolvable: true
  manufacturer_blocklist:
    - 0x004C  # Apple: AirPods, AirTags, Continuity
  service_uuid_allowlist:
    - 0xFFF6                                   # Matter commissioning
    - "00467768-6228-2272-4663-277478268000"   # Improv Wi-Fi
  irks:
    - !secret irk_my_phone
```

## Options

| Option | Default | Effect |
|---|---|---|
| `rssi_threshold` | `-127` | Drop advertisements weaker than this (dBm). `-127` forwards everything |
| `drop_non_resolvable` | `false` | Drop non-resolvable private addresses — they rotate and carry no identity, so nothing can ever match them |
| `irks` | `[]` | Identity Resolving Keys (32 hex chars each) for your own phones/watches. A resolvable private address matching none of them is someone else's and is dropped. Empty disables the check |
| `allow_espressif` | `true` | Exempt Espressif-OUI addresses from the RPA check |
| `mac_allowlist` | `[]` | Addresses that bypass **every** filter, including the RSSI threshold |
| `mac_blocklist` | `[]` | Addresses this proxy ignores entirely. Beats every allow rule |
| `service_uuid_allowlist` | `[]` | Service UUIDs (16-bit or 128-bit) that bypass **every** filter |
| `name_blocklist` | `[]` | Case-insensitive substring match on the advertised local name |
| `manufacturer_blocklist` | `[]` | Bluetooth SIG company identifiers (AD type `0xFF`) |
| `allow_homekit` | `true` | Exempt HomeKit (HAP) from `manufacturer_blocklist` |

Filters run cheapest-first: `mac_blocklist` → `mac_allowlist` → `service_uuid_allowlist` →
RSSI → non-resolvable → unresolved RPA → name/manufacturer payload scan.

## Three options that exist to stop the others breaking things silently

These are not conveniences. Each fixes a way the address filters quietly break a working setup:

- **`service_uuid_allowlist`** — a device in pairing or commissioning mode advertises from a
  *rotating* private address, so `drop_non_resolvable` and the IRK test discard it, and its
  address cannot be allowlisted in advance because it is not knowable. Matching on the
  advertised service UUID is the only handle that exists at that point. Matter commissioning
  is `0xFFF6`; Improv Wi-Fi is the 128-bit case. Without this, turning on the address filters
  makes Matter BLE commissioning impossible, with no error to explain why.
- **`allow_homekit`** — HomeKit-over-BLE advertises under Apple's company id with subtype
  `0x06`. Blocklisting Apple to kill AirPods/AirTag/Continuity noise would take every HomeKit
  accessory with it, and unlike your own phones they carry no IRK to rescue them. On by
  default; AirPods (`0x07`) and Continuity (`0x10`) stay blocked.
- **`mac_blocklist`** — when several proxies are in range of a device that requires bonding,
  only one can hold the bond cleanly and the rest cause connection thrashing. Home Assistant
  picks a proxy from the ones reporting the device, so dropping its advertisements here takes
  this proxy out of the running *without* turning it into a single-purpose bridge. Raised as a
  use case on [esphome/esphome#14353](https://github.com/esphome/esphome/pull/14353) and
  [esphome/feature-requests#2908](https://github.com/esphome/feature-requests/issues/2908).

## Diagnostics

`get_adv_forwarded()`, `get_adv_dropped()`, `get_adv_dropped_rpa()` and
`get_adv_allowed_service_uuid()` expose per-proxy counters, so the effect is measurable rather
than guessed. Surface them as template sensors reporting the delta per update interval to see
a rate.

`get_adv_allowed_service_uuid()` sits at zero unless the service-UUID bypass actually fired,
which makes a failed commissioning attempt diagnosable instead of guesswork.

## Caveats

- **Passive scanning loses local names.** If your tracker runs `active: false`, scan responses
  are gone and many devices put their local name only there — so Home Assistant may not be able
  to discover a *new* BLE device at all. Switch to active scanning while onboarding.
- **A changed IRK is invisible.** IRKs are regenerated when a phone is erased and restored.
  That phone silently stops resolving and its tracker sticks at `not_home`. Alert on it by
  comparing against a non-BLE presence source.

## Credits

Developed with AI assistance (Claude), and tested across a 60+ node ESPHome BLE proxy
deployment spanning ESP32-S3, C5, C6, ESP8685/C3, Shelly and Ethernet boards.
