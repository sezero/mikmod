For XAudio2 support you need DirectX SDK / xaudio2.h, then:
  add DRV_XAUDIO2 among your preprocessor definitions, and
  add ole32.lib among your link libraries.

For OpenAL support you need OpenAL 1.1 SDK, then: add DRV_OPENAL
  among your preprocessor definitions and add OpenAL32.lib among
  your link libraries.  (See drv_openal.c for notes about OpenAL
  header location issues, if necessary. )

