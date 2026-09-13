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
  rssi_threshold: -75
  rssi_floor: -90          # bounds mac_allowlist too
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
| `rssi_threshold` | `-127` | RSSI limit (dBm) for everything not matched by a protection category. `-127` forwards everything |
| `rssi_floor` | `-127` | Absolute reception limit applied to **every** advertisement, allowlisted ones included, before anything is categorised. `-127` disables it |
| `rssi_mac_allowlist` | `-127` | RSSI limit for `mac_allowlist` hits. `-127` = bounded only by `rssi_floor` |
| `rssi_irk` | `-127` | RSSI limit for IRK-matched devices. `-127` = inherit `rssi_threshold` |
| `rssi_service_uuid` | `-127` | RSSI limit for `service_uuid_allowlist` hits. `-127` = inherit `rssi_threshold` |
| `drop_non_resolvable` | `false` | Drop non-resolvable private addresses — they rotate and carry no identity, so nothing can ever match them |
| `irks` | `[]` | Identity Resolving Keys (32 hex chars each) for your own phones/watches. A resolvable private address matching none of them is someone else's and is dropped. Empty disables the check |
| `allow_espressif` | `true` | Exempt Espressif-OUI addresses from the RPA check |
| `mac_allowlist` | `[]` | Addresses that bypass every filter, subject only to `rssi_floor` and `rssi_mac_allowlist` |
| `mac_blocklist` | `[]` | Addresses this proxy ignores entirely. Beats every allow rule |
| `allowlist_exclusive` | `false` | Turn `mac_allowlist` from a bypass list into an exclusive one — nothing else is forwarded |
| `service_uuid_allowlist` | `[]` | Service UUIDs (16-bit or 128-bit) that bypass the payload filters, subject to `rssi_floor` and `rssi_service_uuid` |
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

## Relationship to the esp32_ble hardware whitelist

[esphome/esphome#14353](https://github.com/esphome/esphome/pull/14353) programs the ESP-IDF
controller whitelist (`BLE_SCAN_FILTER_ALLOW_ONLY_WLST`). **Where that is available it is the
better tool** and this component is not a substitute:

- It filters in the **BLE controller**, so non-matching packets never reach the host stack at
  all — saving CPU and power, not just network traffic. A host-side filter like this one
  receives and parses every packet before discarding it.
- It also restricts **which devices may connect** to the ESP32, a security property an
  advertisement filter cannot provide.
- It applies to every consumer, including local `esp32_ble_tracker` sensors, not just what the
  proxy forwards.

`allowlist_exclusive: true` reproduces only the *observable* advertisement behaviour, for the
platforms that PR cannot reach — it is ESP32-only, while `bluetooth_proxy` also runs on
RP2040/RP2350, BK72xx and LN882x. On ESP32, prefer the hardware whitelist for a single-purpose
bridge, and this component when a general-purpose proxy needs to shed noise.

## Diagnostics

`get_adv_forwarded()`, `get_adv_dropped()`, `get_adv_dropped_rpa()`,
`get_adv_allowed_service_uuid()` and `get_adv_dropped_floor()` expose per-proxy counters, so the effect is measurable rather
than guessed. Surface them as template sensors reporting the delta per update interval to see
a rate.

`get_adv_allowed_service_uuid()` sits at zero unless the service-UUID bypass actually fired,
which makes a failed commissioning attempt diagnosable instead of guesswork.

`get_adv_dropped_floor()` is the one counter that can include otherwise protected devices, so a
rising value means `rssi_floor` is cutting a tracked tag — which is exactly when it needs
revisiting.

## Filter order

An advertisement is **categorised first**, then measured against that category's own RSSI
limit, which is what makes the limits independent: any category can be looser or stricter than
any other.

1. `mac_blocklist` hit → drop, ahead of every allow rule
2. RSSI below `rssi_floor` → drop. Global, applies to allowlisted devices too
3. Categorise, first hit wins, cheapest test first:
   `mac_allowlist` → `MAC` · RPA resolving to an IRK → `IRK`
   (an RPA resolving to none → drop) · allowlisted service UUID → `UUID`
   · anything else → `DEFAULT`
4. RSSI below **that category's** limit → drop
5. `allowlist_exclusive` and `DEFAULT` → drop
6. Non-resolvable private address (with `drop_non_resolvable`), unprotected → drop
7. Blocklisted manufacturer id or local name, unprotected → drop
8. Otherwise forward

### Why a floor as well as a threshold

`mac_allowlist` deliberately exempts tracked beacons from `rssi_threshold`, because a weak
reading at one proxy is exactly what places the tag nearer another. But that exemption was
unbounded: a tag heard at the receiver's noise limit was still forwarded, and an RSSI that far
down carries no usable distance information. It does not help a tracker triangulate, it misleads
it. `rssi_floor` bounds the exemption without removing it.

An unset (`-127`) category limit **inherits**, chosen so a config that sets none of them behaves
exactly as it did before these existed: `mac_allowlist` is bounded only by `rssi_floor`; `irk`
and `service_uuid` inherit `rssi_threshold`. Configuration validation rejects a floor above
`rssi_threshold`, and a category limit below the floor (which could never fire).

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
