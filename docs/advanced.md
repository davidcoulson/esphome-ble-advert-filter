# How the filter decides, and tuning tips

You don't need any of this to use the component. It's here for when you want to know exactly
why something was or wasn't forwarded.

## The exact order

The [diagram in the README](../README.md#what-it-can-do) is the short version. In full, each
advert goes through these steps, and the first one that drops it wins:

1. **Blocked address.** On `mac_blocklist`: dropped.
2. **Too weak for anything.** Weaker than the loosest limit anywhere in your config: dropped.
   See [the early check](#the-early-check) below.
3. **Sort it into a group.** The first match wins:
   - on `mac_allowlist`: *allowlisted*
   - a rotating address that matches one of your `irks`: *yours*
   - matches an `allow_ibeacon` rule: *your beacon*
   - a FindMy advert, with `allow_findmy` on: *FindMy*
   - advertises a UUID on `service_uuid_allowlist`: *pairing*
   - anything else: *everything else*
4. **Someone else's phone.** A rotating address that matched none of your IRKs, and wasn't
   claimed by a group in step 3: dropped. This happens regardless of distance.
5. **Distance.** Weaker than its group's limit: dropped. The limits are:

   | Group | Limit |
   |---|---|
   | allowlisted | `rssi_mac_allowlist`, otherwise only `rssi_floor` |
   | yours | `rssi_irk`, otherwise `rssi_threshold` |
   | pairing | `rssi_service_uuid`, otherwise `rssi_threshold` |
   | your beacon, FindMy | the rule's own `rssi`, otherwise `rssi_threshold` |
   | everything else | `rssi_threshold` |

   `rssi_floor` also applies to every group, except a beacon or FindMy rule that sets its own
   `rssi`.
6. **Exclusive mode.** `allowlist_exclusive` is on and the group is *everything else*: dropped.
7. **Untrackable address.** `drop_non_resolvable` is on, the address is non-resolvable, and the
   group is *everything else*: dropped.
8. **Blocked content.** The manufacturer is on `manufacturer_blocklist` or the name matches
   `name_blocklist`, and the group is *everything else*: dropped. HomeKit accessories are skipped
   here while `allow_homekit` is on.
9. Otherwise it's sent to Home Assistant.

Every group except *everything else* skips steps 6 to 8. That's what "your devices are never
treated as noise" means.

## Choosing limits

A good starting point for a house:

```yaml
  rssi_threshold: -80
  rssi_floor: -90
```

Then watch the sensors for a day:

- If room tracking gets worse, a proxy is probably dropping readings Bermuda needed. Loosen
  that proxy's threshold with the dial, or give your devices their own limit with `rssi_irk`.
- If `dropped_floor` climbs, the floor is cutting off devices you track. Lower it.
- For comparison, one of my proxies, with IRKs set and Apple blocked, sits at about 60%
  `drop_rate`. Yours will depend on how busy your area is.

Why have a floor as well as a threshold? Devices on `mac_allowlist` ignore the threshold on
purpose: a weak reading at one proxy is what tells a tracker the device is nearer another. But
a reading right at the edge of what the radio can hear tells it nothing useful, and can mislead
it. The floor stops that without taking the exemption away.

## The early check

Step 2 works out the weakest signal any of your rules could ever let through, and drops
anything weaker before doing the rest. It saves work on the steady stream of distant adverts.
It's worked out automatically and kept up to date when you change limits at runtime, for
example from the threshold dial. The `dropped_gate` sensor counts what it drops.

Setting any rule's `rssi` to -127 (forward at any strength) switches this check off, so prefer
a real value like -95.

## Performance

Checking a rotating address against your IRKs is the only expensive step: one AES calculation
per IRK. A phone keeps the same address for about 15 minutes and advertises several times a
second, so the filter remembers the answer for the last 64 addresses it checked and works each
one out once per rotation, not once per advert. Changing the IRK list clears that memory, so a
new phone is recognised immediately. The memory is 512 bytes and only exists when you use IRKs.

Everything else is a few comparisons per advert, and the steps that read the advert's contents
only run when you've set the options that need them.

## Compared with the ESP32 hardware filter

ESPHome has a proposal ([esphome/esphome#14353](https://github.com/esphome/esphome/pull/14353),
not merged at the time of writing) to use the ESP32 Bluetooth controller's own allowlist.
Where it's available, that's better for a proxy dedicated to a few devices: unwanted packets
never reach the CPU at all, and it also limits which devices can connect.

This component is for a general-purpose proxy that should forward most things and shed the
noise, which the hardware allowlist can't do. `allowlist_exclusive` gives you the hardware
allowlist's behaviour on proxies that don't have one.

## Tests

```bash
tests/run.sh
```

Builds the real filter code against small stand-ins for ESPHome and runs it under
AddressSanitizer and UBSan. It needs only a C++17 compiler. It covers:

- IRK matching, checked against the Bluetooth specification's own sample data
- every step above, the Apple exceptions, iBeacon rules, all eight ways a service UUID can
  appear in an advert, and runtime IRK changes
- the IRK cache: one check per address, cleared when keys change
- that the early check never changes a decision (over 200,000 comparisons across 400 random
  configs)
- 200,000 random and broken adverts, with no out-of-bounds reads
- the sensors (per-minute rates, drop rate when idle), the threshold dial (saving, restoring
  after a reboot, clamping) and the `set_irks` / `clear_irks` actions

GitHub Actions also builds `tests/compile/` with real ESPHome for ESP32-C3 and ESP32-S3: on the
newest release or beta, the newest stable release, and weekly against ESPHome's development
branch. That catches ESPHome changes that would break the component.
