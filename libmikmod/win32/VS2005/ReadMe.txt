For XAudio2 support you need xaudio2.h (DirectX SDK June 2010 has
  xaudio2_7), then add DRV_XAUDIO2 among your preprocessor defs.
  If you want xaudio2_8 for Windows 8 then also add DRV_XAUDIO28
  among those defs and make sure that xaudio2.h is from win8 sdk.

For OpenAL support you need OpenAL 1.1 SDK, then: add DRV_OPENAL
  among your preprocessor definitions and add OpenAL32.lib among
  your link libraries.  (See drv_openal.c for notes about OpenAL
  header location issues, if necessary. )

For SSE2 support: add MIKMOD_SIMD among your preprocessor definitions.
(SIMD code is unstable at present: *not* recommended.)

