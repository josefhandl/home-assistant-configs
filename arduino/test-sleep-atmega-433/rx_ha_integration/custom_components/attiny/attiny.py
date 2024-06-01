"""attiny API."""

import httpx

from homeassistant.helpers.update_coordinator import UpdateFailed


class attinyAPI:
    """attiny API."""

    def __init__(self, host: str) -> None:
        """Initialize."""
        self._host = host

    @property
    def host(self) -> str:
        """Return host."""
        return self._host

    async def get_sensors(self):
        """Return data."""
        async with httpx.AsyncClient() as client:
            response = await client.get(f"{self.host}/sensors")
            if response.status_code == 200:
                return response.json()
            else:
                raise UpdateFailed(f"HTTP status code {response.status_code}")
