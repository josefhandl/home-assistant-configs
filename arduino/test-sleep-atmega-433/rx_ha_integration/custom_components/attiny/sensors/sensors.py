"""Definitions of all Attiny sensors supoorted by attiny integration."""

from homeassistant.components.binary_sensor import (
    BinarySensorDeviceClass,
    BinarySensorEntity,
)
from homeassistant.components.sensor import (
    SensorDeviceClass,
    SensorEntity,
    SensorStateClass,
)
from homeassistant.const import TEMP_CELSIUS
from homeassistant.core import callback
from homeassistant.helpers.device_registry import DeviceInfo
from homeassistant.helpers.update_coordinator import (
    CoordinatorEntity,
    DataUpdateCoordinator,
)

from ..const import DOMAIN


class AttinySensorBase(CoordinatorEntity):
    """Shared basic structure for all Attiny sensor entities."""

    _name_template: str = "{} sensor"
    _attr_name: str | None
    _sensor_posfix = ""
    _attr_device_info: DeviceInfo | None = None
    _attr_unique_id: str | None = None

    def __init__(
        self, sensor_id, coordinator: DataUpdateCoordinator
    ) -> None:
        """Entity is identified by Attinys serial number, path can change."""
        super().__init__(coordinator, sensor_id)
        self.sensor_id = sensor_id
        self._attr_name = self._name_template.format(sensor_id)
        self._path = sensor_id

        # TODO: Include server id in unique id somehow
        self._attr_unique_id = (
            f"{DOMAIN}_sensor_{str(sensor_id)}_{self._sensor_posfix}"
        )

        self._attr_device_info = DeviceInfo(
            identifiers={(DOMAIN, self.sensor_id)},
            name=f"attiny {self._path}",
            manufacturer="josefhandl",
            model="Carbon",
            sw_version="0.0.1",
        )


class AttinySensor(AttinySensorBase, SensorEntity):
    """SensorEntity extended of Attiny basic functions."""

    pass


class AttinyBinnarySensor(AttinySensorBase, BinarySensorEntity):
    """BinarySensorEntity extended of Attiny basic functions."""

    pass


class AttinyCyclesSensor(AttinySensor):
    """Representation of a Sensor."""

    _attr_name = "Attiny cycles"
    _name_template = "Attiny {} cycles"
    #_attr_native_unit_of_measurement = TEMP_CELSIUS
    #_attr_device_class = SensorDeviceClass.TEMPERATURE
    _attr_state_class = SensorStateClass.MEASUREMENT
    _sensor_posfix = "cycles"

    @callback
    def _handle_coordinator_update(self) -> None:
        """Handle updated data from the coordinator."""
        for sensor in self.coordinator.data.get("sensors"):
            if sensor.get("id") == self.sensor_id:
                self._attr_native_value = sensor.get("sleeps", None)
        self.async_write_ha_state()


class AttinyCapVoltageSensor(AttinySensor):
    """Representation of a Sensor."""

    _attr_name = "Attiny capVoltage"
    _name_template = "Attiny {} capVoltage"
    _attr_native_unit_of_measurement = "V"
    _attr_device_class = SensorDeviceClass.VOLTAGE
    _attr_state_class = SensorStateClass.TOTAL
    _sensor_posfix = "capVoltage"

    @callback
    def _handle_coordinator_update(self) -> None:
        """Handle updated data from the coordinator."""
        for sensor in self.coordinator.data.get("sensors"):
            if sensor.get("id") == self.sensor_id:
                self._attr_native_value = sensor.get("capVoltage", None)
        self.async_write_ha_state()


class AttinyMoistureSensor(AttinySensor):
    """Representation of a Sensor."""

    _attr_name = "Attiny moisture"
    _name_template = "Attiny {} moisture"
    _attr_native_unit_of_measurement = "%"
    _attr_device_class = SensorDeviceClass.MOISTURE
    _attr_state_class = SensorStateClass.MEASUREMENT
    _sensor_posfix = "moisture"

    @callback
    def _handle_coordinator_update(self) -> None:
        """Handle updated data from the coordinator."""
        for sensor in self.coordinator.data.get("sensors"):
            if sensor.get("id") == self.sensor_id:
                self._attr_native_value = sensor.get("moisture", None)
        self.async_write_ha_state()
