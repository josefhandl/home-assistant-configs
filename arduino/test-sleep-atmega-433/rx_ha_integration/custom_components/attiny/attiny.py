"""attiny API."""

import httpx

from homeassistant.core import HomeAssistant
from homeassistant.helpers.httpx_client import get_async_client
from homeassistant.helpers.update_coordinator import UpdateFailed


class AttinyAPI:
    """attiny API."""

    def __init__(self, hass: HomeAssistant, host: str) -> None:
        """Initialize."""
        self._hass = hass
        self._host = host

    @property
    def host(self) -> str:
        """Return host."""
        return self._host

    async def get_sensors(self):
        """Return data."""
        async with get_async_client(self._hass) as client:
            response = await client.get(f"{self.host}/sensors")
            if response.status_code == 200:
                return response.json()
            else:
                raise UpdateFailed(f"HTTP status code {response.status_code}")
