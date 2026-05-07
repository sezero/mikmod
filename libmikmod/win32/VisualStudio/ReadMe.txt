* WASAPI support is enabled in the project file.

* XAudio2 support for XAudio2 >= 2.8 is enabled in the project file.

* For OpenAL support you need OpenAL 1.1 SDK: Add DRV_OPENAL among
  your preprocessor definitions, and add OpenAL32.lib among your
  link libraries. (See drv_openal.c for notes about OpenAL header
  location issues, if necessary. )

* For SSE2 support: Add MIKMOD_SIMD among your preprocessor definitions.
  (the SIMD code is unstable at present: *NOT* recommended.)

