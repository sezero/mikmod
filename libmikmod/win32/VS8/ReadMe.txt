* For XAudio2 support you need xaudio2.h (DirectX SDK June 2010 has
  xaudio2_7). Add DRV_XAUDIO2 among your preprocessor definitions.

* For OpenAL support you need OpenAL 1.1 SDK: Add DRV_OPENAL among
  your preprocessor definitions, and add OpenAL32.lib among your
  link libraries. (See drv_openal.c for notes about OpenAL header
  location issues, if necessary. )

* For SSE2 support: Add MIKMOD_SIMD among your preprocessor definitions.
  (the SIMD code is unstable at present: *NOT* recommended.)

