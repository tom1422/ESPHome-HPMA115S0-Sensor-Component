import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import (CONF_ID, CONF_PM_10_0, CONF_PM_2_5, CONF_RX_ONLY,
                           CONF_UPDATE_INTERVAL, UNIT_MICROGRAMS_PER_CUBIC_METER,
                           ICON_CHEMICAL_WEAPON)

DEPENDENCIES = ['uart']

hpma115s0_esphome_ns = cg.esphome_ns.namespace('hpma115s0_esphome')
hpma115S0CustomSensor = hpma115s0_esphome_ns.class_('hpma115S0CustomSensor', cg.PollingComponent, uart.UARTDevice)

CONF_PM_2_5 = "pm_2_5"
CONF_PM_10_0 = "pm_10_0"

CONFIG_SCHEMA = uart.UART_DEVICE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(hpma115S0CustomSensor),
    cv.Optional(CONF_PM_2_5): sensor.sensor_schema(UNIT_MICROGRAMS_PER_CUBIC_METER, ICON_CHEMICAL_WEAPON, 1),
    cv.Optional(CONF_PM_10_0): sensor.sensor_schema(UNIT_MICROGRAMS_PER_CUBIC_METER, ICON_CHEMICAL_WEAPON, 1),
}).extend(cv.polling_component_schema('60s'))


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
    
    if CONF_PM_2_5 in config:
        sens = yield sensor.new_sensor(config[CONF_PM_2_5])
        cg.add(var.set_pm_2_5_sensor(sens))

    if CONF_PM_10_0 in config:
        sens = yield sensor.new_sensor(config[CONF_PM_10_0])
        cg.add(var.set_pm_10_0_sensor(sens))