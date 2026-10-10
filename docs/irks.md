# Getting your IRKs

Phones and watches don't advertise from a fixed Bluetooth address. They switch to a new random
one every 15 minutes or so, which stops strangers from following them around. That also means
a proxy can't tell your phone from your neighbour's just by looking at the address.

An IRK (Identity Resolving Key) is the secret that lets a device you've paired with recognise
those random addresses. Give the filter the IRK of each phone and watch you care about, and it
can keep yours and drop everyone else's.

If you already use the [Private BLE Device](https://www.home-assistant.io/integrations/private_ble_device/)
integration or [Bermuda](https://github.com/agittins/bermuda) with IRKs, you have these
already. It's the same 32-character key.

## Capture them with irk-capture

The easiest way is Derek Seaman's [irk-capture](https://github.com/DerekSeaman/irk-capture).
You flash it to a spare ESP32, pair your phone or watch with it, and it shows the IRK in Home
Assistant. It works for iPhones, Apple Watches and most Android phones and watches.

1. Flash irk-capture to any spare ESP32. It can't run as a Bluetooth proxy at the same time,
   so use a board you can set aside, or flash it temporarily.
2. Pair your phone or watch with it. Derek's
   [step-by-step guide](https://www.derekseaman.com/2026/01/how-to-using-my-bluetooth-irk-capture-package.html)
   covers each type of device.
3. Copy the value from its **IRK** sensor in Home Assistant. It's 32 hexadecimal characters.
4. Repeat for each device.

IRKs don't change unless the device is erased or restored from a backup, so this is a one-off
job per device.

## Put them in your proxy config

The simplest option: list them in the proxy's YAML, using ESPHome secrets so the keys stay out
of your config files.

```yaml
# secrets.yaml (ESPHome's)
irk_my_phone: "0123456789abcdef0123456789abcdef"
irk_my_watch: "fedcba9876543210fedcba9876543210"
```

```yaml
ble_advert_filter:
  irks:
    - !secret irk_my_phone
    - !secret irk_my_watch
```

Spaces, colons and dashes are fine. If something gave you an IRK in base64 instead of hex,
convert it to hex first.

Once IRKs are set, an advert from a rotating private address that matches none of them is
dropped.
Check the proxy's log after flashing: it shows how many IRKs loaded.

## Loading IRKs from Home Assistant

With the keys in each proxy's YAML, adding a phone means reflashing every proxy. If you'd
rather edit one list in Home Assistant, keep it there and have each proxy subscribe to it.

**In Home Assistant**, keep the list in HA's own `secrets.yaml`, with a name next to each key:

```yaml
# /config/secrets.yaml (Home Assistant's, not ESPHome's)
ble_proxy_irks: |
  My phone:  0123456789abcdef0123456789abcdef
  My watch:  fedcba9876543210fedcba9876543210
```

Then publish it as an attribute of a template sensor, and keep it out of history:

```yaml
# for example /config/packages/ble_proxy_irks.yaml
template:
  - sensor:
      - name: "BLE Proxy IRKs"
        unique_id: ble_proxy_irks
        icon: mdi:key-chain
        state: "configured"
        attributes:
          irks: !secret ble_proxy_irks

recorder:
  exclude:
    entities:
      - sensor.ble_proxy_irks
```

It has to be an attribute, not the state: Home Assistant limits a state to 255 characters, and
eight keys are already 256.

**On each proxy**:

```yaml
text_sensor:
  - platform: homeassistant
    entity_id: sensor.ble_proxy_irks
    attribute: irks
    internal: true
    on_value:
      - ble_advert_filter.set_irks: !lambda "return x;"

sensor:
  - platform: ble_advert_filter
    irk_count:
      name: "BLE IRKs Loaded"
```

After reloading template entities in Home Assistant, `BLE IRKs Loaded` on each proxy should
show the number of keys in your list. Changes reach every proxy within a second or two, with
no reboot.

### How the list is read

- Every run of exactly 32 hex characters is taken as a key, and everything else is ignored. The
  names are just for you.
- A name must not itself contain 32 hex characters in a row.
- Two keys need something between them. Sixty-four hex digits in a row are rejected, not split.
- Up to 32 keys are loaded; the rest are ignored with a warning in the log.
- If the entity has no valid key at all (for example it's `unavailable` while Home Assistant
  restarts), the proxy keeps the list it already has. Your phones won't be dropped because Home
  Assistant blinked.
- To empty the list on purpose, use the `ble_advert_filter.clear_irks` action.

After a reboot, the proxy uses the `irks:` from its YAML until Home Assistant connects and sends
the list, which usually takes a few seconds. With no `irks:` in the YAML, nobody's phone is
dropped as a stranger in that gap. But if you block Apple (`manufacturer_blocklist: [0x004C]`),
your own iPhones are caught by that block until the list arrives. If that gap matters to you,
list your keys in the YAML as well; the list from Home Assistant replaces it once it arrives.

## Checking it works

Add these sensors to a proxy:

```yaml
sensor:
  - platform: ble_advert_filter
    forwarded_irk:
      name: "BLE IRK Matches"
    dropped_rpa:
      name: "BLE Other People's Phones Dropped"
```

With your phone near that proxy, `BLE IRK Matches` should be above zero. If it stays at zero
while you're home, the key is probably wrong or out of date: capture it again.

## When a phone gets a new IRK

Erasing a phone, or restoring it from a backup, gives it a new IRK. The old one stops matching
and that phone's adverts are dropped like a stranger's. Capture the new IRK and replace the old
one, in the YAML or in Home Assistant.
