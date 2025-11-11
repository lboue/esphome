/* C wrapper: include the core BMI2 implementation so it's compiled as a C
  translation unit by PlatformIO. We include only bmi2.c here because it
  defines the core APIs used by the driver (bmi2_get_sensor_data, init,
  temperature helpers, etc.). Keep this file as a single .c TU to avoid
  C++ parsing issues. */
#include "/workspaces/esphome/esphome/components/bmi270/bmi2/bmi2.c"
