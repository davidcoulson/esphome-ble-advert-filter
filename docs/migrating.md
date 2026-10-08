# Migrating from esphome-bluetooth-proxy-filter

[esphome-bluetooth-proxy-filter](https://github.com/davidcoulson/esphome-bluetooth-proxy-filter)
is the same filter, built into a modified copy of ESPHome's `bluetooth_proxy`. It was the only
way to do this before ESPHome 2026.10. From 2026.10, stock `bluetooth_proxy` has a hook for
filters, and this component plugs into it, so there's no modified core component to keep in
step with every ESPHome release.

The options and behaviour are the same. The old fork stays available for ESPHome 2026.9 and
earlier but won't get new features. Its v1.7.0 matches this repo's v1.7.0; the IRK cache,
sensors, threshold dial and actions are only here, from v2.0.0.

## The change

Once a proxy is on ESPHome 2026.10:

```yaml
external_components:
  # remove the old one:
  # - source: { type: git, url: https://github.com/davidcoulson/esphome-bluetooth-proxy-filter, ref: v1.7.0 }
  #   components: [bluetooth_proxy]
  - source:
      type: git
      url: https://github.com/davidcoulson/esphome-ble-advert-filter
      ref: v2.0.0
    components: [ble_advert_filter]

bluetooth_proxy:
  id: ble_proxy_core       # a new id; the old one moves to the filter below
  active: true             # active, cache_services and connection_slots stay here

ble_advert_filter:
  id: ble_proxy            # the id your lambdas already use
  rssi_threshold: -80      # every filter option moves here, unchanged
  irks:
    - !secret irk_my_phone
```

Giving the filter the id your old proxy had means existing lambdas such as
`id(ble_proxy).set_rssi_threshold(x)`, `get_adv_forwarded()` and `set_irks(x)` keep working
without changes.

## Afterwards

You can replace template sensors and numbers built on those lambdas with the built-in ones, at
your own pace:

- template counter sensors: the [sensor platform](options.md#sensors)
- a template number calling `set_rssi_threshold`: the [threshold dial](options.md#threshold-dial-number)
- a lambda calling `set_irks(x)`: the [`ble_advert_filter.set_irks` action](options.md#actions)

If you keep the threshold on a template number with `restore_value`, it keeps working as it
did. If you switch to the built-in dial, it starts from the YAML `rssi_threshold` the first
time, since it saves its value separately.
