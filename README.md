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
| `allow_ibeacon` | `false` | Exempt iBeacons from `manufacturer_blocklist` — `true` for all, or a list of `major`/`minor`/`rssi` filters |
| `allow_findmy` | `false` | Exempt Apple FindMy accessories (AirTags, AirPods, licensed tags) from `manufacturer_blocklist` — `true`, or `{rssi: N}` to give them their own limit |

The order the filters run in, and why, is under [Filter order](#filter-order).

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

`get_adv_forwarded()`, `get_adv_dropped()`, `get_adv_dropped_rpa()`, `get_adv_forwarded_irk()`,
`get_adv_allowed_service_uuid()`, `get_adv_dropped_floor()` and `get_adv_dropped_gate()`
expose per-proxy counters, so the effect is measurable rather
than guessed. Surface them as template sensors reporting the delta per update interval to see
a rate.

`get_adv_allowed_service_uuid()` sits at zero unless the service-UUID bypass actually fired,
which makes a failed commissioning attempt diagnosable instead of guesswork.

`get_adv_forwarded_irk()` counts advertisements forwarded because their address resolved to one
of your IRKs. It is the only way to tell a wrong key from an absent phone: a key that never
matches leaves it flat while its owner's adverts are counted by `get_adv_dropped_rpa()` as
somebody else's. `get_irk_count()` is how many keys are loaded.

`get_adv_dropped_floor()` is the one counter that can include otherwise protected devices, so a
rising value means `rssi_floor` is cutting a tracked tag — which is exactly when it needs
revisiting.

## Changing IRKs without reflashing

`irks:` is compile-time, so on its own a new phone means reflashing every
proxy. `set_irks()` replaces the list at runtime; the usual source is a Home
Assistant entity each proxy subscribes to:

```yaml
text_sensor:
  - platform: homeassistant
    entity_id: sensor.ble_proxy_irks
    attribute: irks            # an attribute: HA caps states at 255 characters
    internal: true
    on_value:
      - lambda: 'id(ble_proxy).set_irks(x);'
```

The parser takes every run of **exactly 32 hex characters** and ignores the
rest, so the Home Assistant side can keep a name next to each key -
`David phone: 0011...`, or a stringified dict - and a replacement is an edit
to one named line. The only rule is that no label contains 32 consecutive hex
digits. Runs of 31 or 33 are rejected rather than trimmed, and two keys with
no separator between them (64 digits) are rejected rather than split.

**Input with no valid key leaves the current list untouched** and returns
`-1`. An entity that is briefly `unavailable` during a Home Assistant restart
must not be able to wipe the list: with `manufacturer_blocklist: [0x004C]` a
wiped list silently drops your own phones. `clear_irks()` empties it on
purpose.

The compile-time list still loads in `setup()`; `set_irks()` replaces it, it
does not merge. `get_irks()` exposes the live list read-only so a lambda can
persist it to flash for use before Home Assistant connects. Calls are safe at
any time - advertisements and API state updates are both dispatched from the
main loop, so the list is never swapped mid-lookup.

## Exempting FindMy accessories

AirTags, AirPods and licensed third-party FindMy tags advertise Apple
manufacturer data with the Offline Finding subtype `0x12`, from a random
static address. AirPods near their owner send the proximity-pairing subtype
`0x07` instead - the advert that raises the AirPods card on an iPhone - but
from the same rotated address, so `allow_findmy` exempts that subtype too.
Without it an AirPods case sitting on a desk is heard only by receivers that
run no Apple blocklist. Neither the IRK test nor `drop_non_resolvable`
touches them, so with `manufacturer_blocklist: [0x004C]` the Apple entry is
the only thing dropping them - and it drops every one. A tracker that holds an
accessory's pairing keys (Bermuda's FindMy support) can follow its address
rotation, but only if the adverts reach it:

```yaml
ble_advert_filter:
  manufacturer_blocklist: [0x004C]
  allow_findmy: true            # inherits rssi_threshold, bounded by rssi_floor
  # or, with its own limit (overrides both, like an iBeacon rule):
  allow_findmy:
    rssi: -85
```

There is no per-accessory scoping: the advert carries no identity a proxy
could act on (the key material that resolves the rotation lives in the
tracker), so every FindMy accessory in range comes through. Bound it with
`rssi` rather than the fleet threshold when neighbours' tags are the concern.

## Exempting iBeacons

iBeacon is Apple manufacturer data with subtype `0x02`, so a
`manufacturer_blocklist: [0x004C]` entry — the usual way to kill AirPods, AirTag and
neighbour noise — silently takes **every iBeacon** with it. `allow_homekit` does not help:
that exempts subtype `0x06`.

This matters if your own ESPHome nodes beacon, for example for BLE positioning
self-calibration: those adverts come from the node's public Espressif MAC, so they carry no
IRK and nothing else rescues them.

```yaml
ble_advert_filter:
  manufacturer_blocklist: [0x004C]
  allow_ibeacon: true          # every iBeacon exempt
```

Scope it to your own beacons instead of opening the door to every iBeacon in radio range:

```yaml
  allow_ibeacon:
    - major: 1
      minor: 7                 # one probe
    - major: 10
      minor: [3, 4, 5]         # several
    - major: 11                # whole major, any minor
```

Each filter can carry its own `rssi`, which **overrides both `rssi_threshold` and
`rssi_floor`** for adverts it matches:

```yaml
  allow_ibeacon:
    - major: 1
      rssi: -127               # our probes: forward at any strength
    - major: 10
      rssi: -85
```

That is the point of the feature. Probe-to-probe ranging wants exactly the weak cross-room
readings the fleet threshold exists to discard — and only for the beacons doing the ranging.

**Omitting `rssi` inherits**: the filter exempts the advert from `manufacturer_blocklist`
and nothing else, so `rssi_threshold` and `rssi_floor` still apply. Omitting a value is
never the most permissive setting. A bare `allow_ibeacon: true` behaves the same way.

`rssi: -127` disables the pre-gate below, so prefer a real value like `-95` unless you
genuinely want everything.

The filters **narrow** the exemption; they never add a drop rule. An iBeacon matching none
of them falls through to the normal manufacturer test, exactly as if the exemption were
off. A truncated iBeacon carrying no major/minor cannot be matched, so it is not exempted
when filters are in use (bare `true` still exempts it).

### The pre-gate

Every advert is measured against *some* limit, so anything weaker than the most permissive
limit in the config is dropped before categorisation runs. That keeps an AES resolve and a
payload walk off every distant advert once your own beacons are allowed through at a low
RSSI. It is derived automatically from `rssi_threshold`, `rssi_floor`, every per-category
limit and every rule that brings its own — but only for categories something can actually
land in: with no `mac_allowlist` there is no allowlisted advert for the gate to protect, so
that category does not hold it open. A reachable category with no bound at all disables it,
correctly — if something is allowed through at any strength, nothing can be rejected on RSSI
alone.

It is recomputed whenever a limit or list changes, **including at runtime**. It used to be
computed once at codegen, so a Home Assistant number lowering `rssi_threshold` below the
compiled gate silently stopped working. `get_min_rssi_gate()` reports the current value.

`get_adv_dropped_gate()` counts what it rejects, separately from
`get_adv_dropped_floor()`: the gate means "too weak for **any** rule", the floor means "too
weak for this category".

## Filter order

An advertisement is **categorised first**, then measured against that category's own RSSI
limit, which is what makes the limits independent: any category can be looser or stricter than
any other.

1. `mac_blocklist` hit → drop, ahead of every allow rule
2. RSSI below the pre-gate → drop. Global, applies to allowlisted devices too
3. Categorise, first hit wins, cheapest test first:
   `mac_allowlist` → `MAC` · RPA resolving to an IRK → `IRK` · matched iBeacon → `IBEACON`
   · FindMy (with `allow_findmy`) → `FINDMY` · allowlisted service UUID → `UUID`
   · anything else → `DEFAULT`
   An RPA that resolved to **none** of your IRKs is dropped here, regardless of RSSI — but
   only if no other rule claimed it. It used to be dropped before the payload rules ran,
   which meant `service_uuid_allowlist` could not rescue a device pairing from a resolvable
   address, the case it exists for.
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
`rssi_threshold` (when one is set — a floor on its own is fine), and a category limit below the
floor (which could never fire).

## Caveats

- **Passive scanning loses local names.** If your tracker runs `active: false`, scan responses
  are gone and many devices put their local name only there — so Home Assistant may not be able
  to discover a *new* BLE device at all. Switch to active scanning while onboarding.
- **A changed IRK is nearly invisible.** IRKs are regenerated when a phone is erased and
  restored. That phone silently stops resolving and its tracker sticks at `not_home`.
  `get_adv_forwarded_irk()` going flat while the phone is home is the on-device sign; otherwise
  compare against a non-BLE presence source. Replacing the key needs no reflash — see
  [Changing IRKs without reflashing](#changing-irks-without-reflashing).

## Tests

```bash
tests/run.sh
```

Compiles the **real** `ble_advert_filter.cpp` against small stubs (`tests/stubs/`) under
AddressSanitizer and UBSan — nothing is transcribed, so the tests cannot drift from the
component. Needs only a C++17 compiler; CI runs it on every push.

- IRK resolution checked against the Bluetooth Core spec's own sample data, with an AES written
  independently of ESPHome's
- every stage of the chain, the Apple carve-outs, iBeacon rules, service-UUID passthrough in
  all eight AD forms, runtime IRKs
- a property test that the pre-gate never changes a verdict (150,000+ comparisons across 400
  random configs)
- 200,000 random and truncated payloads, each in an exactly sized heap buffer so a one-byte
  overread aborts the run

## Migrating from the `bluetooth_proxy` fork

[esphome-bluetooth-proxy-filter](https://github.com/davidcoulson/esphome-bluetooth-proxy-filter)
is the same filter inside a fork of `bluetooth_proxy`. The two are kept textually identical
(that repo's `tools/check_parity.py` fails CI on drift), with the same options and the same
public methods, so moving is mechanical once you are on ESPHome 2026.10:

```yaml
external_components:
  # was: .../esphome-bluetooth-proxy-filter, components: [bluetooth_proxy]
  - source: github://davidcoulson/esphome-ble-advert-filter@<tag>
    components: [ble_advert_filter]

bluetooth_proxy:
  id: ble_proxy_core        # stock component again; only its own options stay here
  active: true

ble_advert_filter:
  id: ble_proxy             # <- the id your lambdas already use
  rssi_threshold: -80       # every filter option moves here, unchanged
  irks: [...]
```

Giving the filter the id the fork's proxy had means existing lambdas —
`id(ble_proxy).set_rssi_threshold(x)`, `get_adv_forwarded()`, `set_irks(x)` — keep working
untouched. `connection_slots`, `active` and `cache_services` stay under `bluetooth_proxy:`.

## Credits

Developed with AI assistance (Claude), and tested across a 60+ node ESPHome BLE proxy
deployment spanning ESP32-S3, C5, C6, ESP8685/C3, Shelly and Ethernet boards.
