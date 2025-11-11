## Changelog

- 2025-11-11: Vendored Bosch BMI2 SDK into the `bmi270` component and integrated the BMI2 runtime API.
  - Added guarded I2C wrappers so the BMI2 C callbacks cannot dereference an uninitialized bus pointer.
  - Retained legacy register-based initialization as a safe fallback when BMI2 init/config fails.
  - Added component tests to validate config generation and compilation on ESP32 IDF targets.

This change was made to ensure reliable initialization of BMI270 devices that require the BMI2 firmware/config upload.
