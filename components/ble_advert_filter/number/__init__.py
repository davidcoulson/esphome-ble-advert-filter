"""rssi_threshold as a Home Assistant number, tunable without reflashing."""

import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import (
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_RESTORE_VALUE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_DECIBEL_MILLIWATT,
)
from esphome.core import CORE
import esphome.final_validate as fv
from esphome.types import ConfigType

from .. import (
    CONF_BLE_ADVERT_FILTER_ID,
    CONF_RSSI_FLOOR,
    CONF_RSSI_THRESHOLD,
    BLEAdvertFilter,
    ble_advert_filter_ns,
)

DEPENDENCIES = ["ble_advert_filter"]

RSSIThresholdNumber = ble_advert_filter_ns.class_(
    "RSSIThresholdNumber", number.Number, cg.Component
)

# Without an rssi_floor the dial bottoms out here: below about -100 dBm a
# reading is noise, so lower settings would only look like they did something.
_DEFAULT_MIN = -100


def _filter_floor() -> int:
    """The filter's rssi_floor, or -127 when it has none."""
    return fv.full_config.get().get("ble_advert_filter", {}).get(CONF_RSSI_FLOOR, -127)


def _validate_range(config: ConfigType) -> ConfigType:
    conf = config.get(CONF_RSSI_THRESHOLD)
    if conf is None:
        return config
    floor = _filter_floor()
    minimum = conf.get(CONF_MIN_VALUE)
    if minimum is not None and floor != -127 and minimum < floor:
        # rssi_threshold only limits the default category, and the floor has
        # already dropped everything below it - so dial positions under the
        # floor would appear to loosen the filter and change nothing.
        raise cv.Invalid(
            f"{CONF_MIN_VALUE} ({minimum}) is below the filter's {CONF_RSSI_FLOOR} "
            f"({floor}); settings under the floor have no effect. Use "
            f"{CONF_MIN_VALUE}: {floor} or higher, or leave it out (it then "
            f"defaults to the floor).",
            path=[CONF_RSSI_THRESHOLD, CONF_MIN_VALUE],
        )
    resolved_min = minimum if minimum is not None else _resolved_default_min(floor)
    if resolved_min >= conf[CONF_MAX_VALUE]:
        raise cv.Invalid(
            f"{CONF_MIN_VALUE} ({resolved_min}) must be below {CONF_MAX_VALUE} "
            f"({conf[CONF_MAX_VALUE]})",
            path=[CONF_RSSI_THRESHOLD],
        )
    return config


def _resolved_default_min(floor: int) -> int:
    return floor if floor != -127 else _DEFAULT_MIN


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BLE_ADVERT_FILTER_ID): cv.use_id(BLEAdvertFilter),
        cv.Optional(CONF_RSSI_THRESHOLD): number.number_schema(
            RSSIThresholdNumber,
            icon="mdi:signal-distance-variant",
            entity_category=ENTITY_CATEGORY_CONFIG,
            unit_of_measurement=UNIT_DECIBEL_MILLIWATT,
        )
        .extend(
            {
                # Defaults to the filter's rssi_floor (or -100 without one).
                cv.Optional(CONF_MIN_VALUE): cv.int_range(min=-127, max=0),
                cv.Optional(CONF_MAX_VALUE, default=-30): cv.int_range(min=-127, max=0),
                cv.Optional(CONF_RESTORE_VALUE, default=True): cv.boolean,
            }
        )
        .extend(cv.COMPONENT_SCHEMA),
    }
)

FINAL_VALIDATE_SCHEMA = _validate_range


async def to_code(config: ConfigType) -> None:
    conf = config.get(CONF_RSSI_THRESHOLD)
    if conf is None:
        return
    floor = CORE.config.get("ble_advert_filter", {}).get(CONF_RSSI_FLOOR, -127)
    minimum = conf.get(CONF_MIN_VALUE, _resolved_default_min(floor))
    var = await number.new_number(
        conf, min_value=minimum, max_value=conf[CONF_MAX_VALUE], step=1
    )
    await cg.register_component(var, conf)
    await cg.register_parented(var, config[CONF_BLE_ADVERT_FILTER_ID])
    cg.add(var.set_restore_value(conf[CONF_RESTORE_VALUE]))
