"""Platform for sensor integration."""
from __future__ import annotations

from asyncio import timeout
from datetime import timedelta
import logging

from homeassistant import config_entries, core
from homeassistant.const import CONF_HOST
from homeassistant.helpers.update_coordinator import DataUpdateCoordinator

from .sensors.sensors import (
    AttinyCyclesSensor,
    AttinyCapVoltageSensor,
    AttinyTemperatureSensor
)

from .attiny import AttinyAPI

_LOGGER = logging.getLogger(__name__)


class BasicCoordinator(DataUpdateCoordinator):
    """My custom coordinator."""

    def __init__(self, hass, api_callback):
        """Initialize my coordinator."""
        super().__init__(
            hass,
            _LOGGER,
            name="attiny callback coordinator",
            update_interval=timedelta(seconds=60),
        )
        self.callback = api_callback

    async def _async_update_data(self):
        """Fetch data from API endpoint.

        This is the place to pre-process the data to lookup tables
        so entities can quickly look up their data.
        """
        async with timeout(10):
            return await self.callback()


async def async_setup_entry(
    hass: core.HomeAssistant,
    config_entry: config_entries.ConfigEntry,
    async_add_entities,
) -> None:
    """Setups sensors from a config entry created in the integrations UI."""

    host = config_entry.data[CONF_HOST]
    if host[-1] == "/":
        host = host[:-1]
    attiny = AttinyAPI(hass, host)
    coordinator_sensors = BasicCoordinator(hass, attiny.get_sensors)

    await coordinator_sensors.async_config_entry_first_refresh()

    data = coordinator_sensors.data
    for drive in data.get("sensors"):
        sensor_id = drive.get("id")
        async_add_entities(
            [
                AttinyCyclesSensor(
                    sensor_id=sensor_id, coordinator=coordinator_sensors
                ),
                AttinyCapVoltageSensor(
                    sensor_id=sensor_id, coordinator=coordinator_sensors
                ),
                AttinyTemperatureSensor(
                    sensor_id=sensor_id, coordinator=coordinator_sensors
                ),
            ],
            update_before_add=True,
        )
