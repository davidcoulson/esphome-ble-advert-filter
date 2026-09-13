"""On-device advertisement filtering for ESPHome's bluetooth_proxy.

Installs a predicate into the proxy's AdvertisementFilter slot (added in
esphome/esphome#19220), so advertisements are dropped on the device instead of
being forwarded for Home Assistant to discard.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import bluetooth_proxy
from esphome.const import CONF_ID
from esphome.core import MACAddress

# Read by the `component_version` text_sensor platform, if the user adds one.
# A plain constant rather than a registration call, so this component needs no
# dependency on it and there is no codegen ordering to get wrong.
COMPONENT_VERSION = "2026.09.13.0"

DEPENDENCIES = ["bluetooth_proxy"]
CODEOWNERS = ["@davidcoulson"]

ble_advert_filter_ns = cg.esphome_ns.namespace("ble_advert_filter")
BLEAdvertFilter = ble_advert_filter_ns.class_("BLEAdvertFilter", cg.Component)

CONF_BLUETOOTH_PROXY_ID = "bluetooth_proxy_id"
CONF_RSSI_THRESHOLD = "rssi_threshold"
CONF_RSSI_FLOOR = "rssi_floor"
CONF_RSSI_MAC_ALLOWLIST = "rssi_mac_allowlist"
CONF_RSSI_IRK = "rssi_irk"
CONF_RSSI_SERVICE_UUID = "rssi_service_uuid"
CONF_IRKS = "irks"
CONF_ALLOW_ESPRESSIF = "allow_espressif"
CONF_ALLOW_HOMEKIT = "allow_homekit"
CONF_DROP_NON_RESOLVABLE = "drop_non_resolvable"
CONF_NAME_BLOCKLIST = "name_blocklist"
CONF_MAC_ALLOWLIST = "mac_allowlist"
CONF_MAC_BLOCKLIST = "mac_blocklist"
CONF_ALLOWLIST_EXCLUSIVE = "allowlist_exclusive"
CONF_MANUFACTURER_BLOCKLIST = "manufacturer_blocklist"
CONF_SERVICE_UUID_ALLOWLIST = "service_uuid_allowlist"


def _mac_address(value):
    """`cv.mac_address`, but safe to run twice (str -> MACAddress is not idempotent)."""
    if isinstance(value, MACAddress):
        return value
    return cv.mac_address(value)


def _validate_irk(value):
    """One 16-byte Identity Resolving Key, as 32 hex chars.

    Accepts the separator styles people paste and normalises to bare lowercase
    hex; the C++ side parses the concatenated blob once at boot.
    """
    value = cv.string_strict(value)
    stripped = value.replace(":", "").replace("-", "").replace(" ", "").lower()
    if len(stripped) != 32:
        raise cv.Invalid(
            f"IRK must be 16 bytes (32 hex characters), got {len(stripped)}"
        )
    if any(c not in "0123456789abcdef" for c in stripped):
        raise cv.Invalid("IRK must be hexadecimal")
    return stripped


def _validate_service_uuid(value):
    """A 16-bit short (0xFFF6) or a full 128-bit UUID."""
    if isinstance(value, int):
        return cv.hex_uint16_t(value)
    text = cv.string_strict(value).strip()
    stripped = text.replace("-", "").replace(":", "").replace(" ", "").lower()
    if len(stripped) == 32:
        if any(c not in "0123456789abcdef" for c in stripped):
            raise cv.Invalid("128-bit service UUID must be hexadecimal")
        return stripped
    return cv.hex_uint16_t(value)


def _validate_rssi_floor(config):
    """Reject a floor stricter than the limits it is meant to backstop.

    rssi_floor runs before categorisation and applies to every advertisement;
    every other limit is applied afterwards to one category. The floor is
    therefore only meaningful while it is the loosest of them. A floor above a
    category's limit would silently take over as that category's effective
    filter, so fail loudly rather than quietly changing behaviour.
    """
    floor = config[CONF_RSSI_FLOOR]
    threshold = config[CONF_RSSI_THRESHOLD]
    for key in (CONF_RSSI_MAC_ALLOWLIST, CONF_RSSI_IRK, CONF_RSSI_SERVICE_UUID):
        limit = config[key]
        if limit != -127 and floor != -127 and limit < floor:
            raise cv.Invalid(
                f"{key} ({limit}) is below {CONF_RSSI_FLOOR} ({floor}), so it "
                f"can never fire: the floor has already dropped anything that "
                f"weak. Raise it above the floor, or remove it.",
                path=[key],
            )
    if floor != -127 and floor > threshold:
        raise cv.Invalid(
            f"{CONF_RSSI_FLOOR} ({floor}) must be at or below "
            f"{CONF_RSSI_THRESHOLD} ({threshold}): the floor is an absolute "
            f"backstop applied ahead of the allowlists, so a floor stricter "
            f"than the threshold would override it for every device.",
            path=[CONF_RSSI_FLOOR],
        )
    return config


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BLEAdvertFilter),
        cv.GenerateID(CONF_BLUETOOTH_PROXY_ID): cv.use_id(
            bluetooth_proxy.BluetoothProxy
        ),
        cv.Optional(CONF_RSSI_THRESHOLD, default=-127): cv.int_range(min=-127, max=0),
        # Absolute floor, applied before categorisation rather than after it, so
        # it bounds mac_allowlist / service_uuid_allowlist too. -127 disables it
        # and is the default, so an unconfigured build is unchanged.
        cv.Optional(CONF_RSSI_FLOOR, default=-127): cv.int_range(min=-127, max=0),
        # Per-category limits. -127 (default) means inherit; see the setters in
        # ble_advert_filter.h for what each inherits and why.
        cv.Optional(CONF_RSSI_MAC_ALLOWLIST, default=-127): cv.int_range(
            min=-127, max=0
        ),
        cv.Optional(CONF_RSSI_IRK, default=-127): cv.int_range(min=-127, max=0),
        cv.Optional(CONF_RSSI_SERVICE_UUID, default=-127): cv.int_range(
            min=-127, max=0
        ),
        cv.Optional(CONF_IRKS, default=[]): cv.ensure_list(_validate_irk),
        cv.Optional(CONF_ALLOW_ESPRESSIF, default=True): cv.boolean,
        cv.Optional(CONF_ALLOW_HOMEKIT, default=True): cv.boolean,
        cv.Optional(CONF_DROP_NON_RESOLVABLE, default=False): cv.boolean,
        cv.Optional(CONF_NAME_BLOCKLIST, default=[]): cv.ensure_list(
            cv.All(cv.string_strict, cv.Length(min=1, max=29))
        ),
        cv.Optional(CONF_MAC_ALLOWLIST, default=[]): cv.ensure_list(_mac_address),
        cv.Optional(CONF_MAC_BLOCKLIST, default=[]): cv.ensure_list(_mac_address),
        cv.Optional(CONF_ALLOWLIST_EXCLUSIVE, default=False): cv.boolean,
        cv.Optional(CONF_MANUFACTURER_BLOCKLIST, default=[]): cv.ensure_list(
            cv.hex_uint16_t
        ),
        cv.Optional(CONF_SERVICE_UUID_ALLOWLIST, default=[]): cv.ensure_list(
            _validate_service_uuid
        ),
    }
).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = cv.All(CONFIG_SCHEMA, _validate_rssi_floor)


async def to_code(config):
    # Supported way to compile the hook into bluetooth_proxy. Do not emit the
    # define directly - it is an implementation detail of that component and may
    # be renamed (esphome/esphome#19220).
    bluetooth_proxy.enable_advertisement_filter()

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_BLUETOOTH_PROXY_ID])
    cg.add(var.set_parent(parent))

    cg.add(var.set_rssi_threshold(config[CONF_RSSI_THRESHOLD]))
    cg.add(var.set_rssi_floor(config[CONF_RSSI_FLOOR]))
    cg.add(var.set_rssi_mac_allowlist(config[CONF_RSSI_MAC_ALLOWLIST]))
    cg.add(var.set_rssi_irk(config[CONF_RSSI_IRK]))
    cg.add(var.set_rssi_service_uuid(config[CONF_RSSI_SERVICE_UUID]))
    cg.add(var.set_allow_espressif(config[CONF_ALLOW_ESPRESSIF]))
    cg.add(var.set_allow_homekit(config[CONF_ALLOW_HOMEKIT]))
    cg.add(var.set_drop_non_resolvable(config[CONF_DROP_NON_RESOLVABLE]))
    cg.add(var.set_allowlist_exclusive(config[CONF_ALLOWLIST_EXCLUSIVE]))

    if irks := config[CONF_IRKS]:
        cg.add(var.set_irks_hex("".join(irks)))
    for needle in config[CONF_NAME_BLOCKLIST]:
        cg.add(var.add_blocked_name(needle.lower()))
    for mac in config[CONF_MAC_ALLOWLIST]:
        cg.add(var.add_allowed_mac(mac.as_hex))
    for mac in config[CONF_MAC_BLOCKLIST]:
        cg.add(var.add_blocked_mac(mac.as_hex))
    for company in config[CONF_MANUFACTURER_BLOCKLIST]:
        cg.add(var.add_blocked_manufacturer(company))
    for uuid in config[CONF_SERVICE_UUID_ALLOWLIST]:
        if isinstance(uuid, str):
            cg.add(var.add_allowed_service_uuid128(uuid))
        else:
            cg.add(var.add_allowed_service_uuid(uuid))
