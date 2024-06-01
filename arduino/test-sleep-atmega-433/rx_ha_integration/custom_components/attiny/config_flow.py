"""Config flow for attinyAPI."""
# import my_pypi_dependency

# from homeassistant.core import HomeAssistant
# from homeassistant.helpers import config_entry_flow

# from .const import DOMAIN


# async def _async_has_devices(hass: HomeAssistant) -> bool:
#     """Return if there are devices that can be discovered."""
#     # TODO Check if there are any devices that can be discovered in the network.
#     devices = await hass.async_add_executor_job(my_pypi_dependency.discover)
#     return len(devices) > 0


# config_entry_flow.register_discovery_flow(DOMAIN, "attinyAPI", _async_has_devices)


from typing import Any, Optional

import voluptuous as vol

from homeassistant import config_entries
from homeassistant.const import CONF_HOST

from .const import DOMAIN

# TODO: add server name / id
SCHEMA_API = vol.Schema({vol.Required(CONF_HOST): str})


class attinyCustomConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    """attiny integration config flow."""

    data: Optional[dict[str, Any]]

    async def async_step_user(self, user_input: Optional[dict[str, Any]] = None):
        """First step setting up URL of attiny API."""

        errors: dict[str, str] = {}
        if user_input is not None:
            try:
                pass
                # user_input
            except ValueError:
                errors["base"] = "auth"
            if not errors:
                # Input is valid, set data.
                self.data = user_input
                # Return the form of the next step.
                # return await self.async_step_repo()
                return self.async_create_entry(title="attiny API URL", data=self.data)

        return self.async_show_form(
            step_id="user", data_schema=SCHEMA_API, errors=errors
        )
