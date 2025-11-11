Vendor BMI2 SDK integration notes
================================

To integrate the Bosch BMI2 SDK into the bmi270 component during the build,
we include several vendor C source files as C translation units using thin
wrappers located alongside the component sources. These wrappers simply
#include the original .c sources by absolute path so PlatformIO/CMake compiles
them as C, avoiding C++ parsing issues and static symbol collisions that occur
when including multiple C files in a single C++ TU.

Files added:
- bmi2_vendor.c (includes bmi2.c)
- bmi270_vendor.c (includes bmi270.c)
- bmi270_context_vendor.c (includes bmi270_context.c)
- bmi270_dsd_vendor.c (includes bmi270_dsd.c)
- bmi270_maximum_fifo_vendor.c (includes bmi270_maximum_fifo.c)
- bmi270_legacy_vendor.c (includes bmi270_legacy.c)
- bmi2_ois_vendor.c (includes bmi2_ois.c)

Notes & next steps:
- The absolute include paths are used to ensure the build can find the
  vendored sources in the workspace during the in-container build. We should
  later change these to relative includes or adjust component build settings.
- Run a full clean build to ensure symbol resolution and to detect remaining
  link-time or compile-time issues. If duplicate symbols appear, we may need
  to reduce the set of vendor sources or adjust defines.
