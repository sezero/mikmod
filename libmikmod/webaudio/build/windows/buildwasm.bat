@ECHO OFF

SET INCLUDE_DIR=..\..\..\include
SET LIB_DIR=..\..\dist
SET SRC_DIR=..\..\..
SET TEMP_DIR=..\..\src\temp

IF NOT EXIST %LIB_DIR%\ (
	MKDIR %LIB_DIR%
)

IF NOT EXIST %TEMP_DIR%\ (
	MKDIR %TEMP_DIR%
)

SET SRCS=^
%SRC_DIR%\loaders\load_669.c ^
%SRC_DIR%\loaders\load_amf.c ^
%SRC_DIR%\loaders\load_asy.c ^
%SRC_DIR%\loaders\load_dsm.c ^
%SRC_DIR%\loaders\load_far.c ^
%SRC_DIR%\loaders\load_gdm.c ^
%SRC_DIR%\loaders\load_gt2.c ^
%SRC_DIR%\loaders\load_imf.c ^
%SRC_DIR%\loaders\load_it.c ^
%SRC_DIR%\loaders\load_m15.c ^
%SRC_DIR%\loaders\load_med.c ^
%SRC_DIR%\loaders\load_mod.c ^
%SRC_DIR%\loaders\load_mtm.c ^
%SRC_DIR%\loaders\load_okt.c ^
%SRC_DIR%\loaders\load_s3m.c ^
%SRC_DIR%\loaders\load_stm.c ^
%SRC_DIR%\loaders\load_stx.c ^
%SRC_DIR%\loaders\load_ult.c ^
%SRC_DIR%\loaders\load_umx.c ^
%SRC_DIR%\loaders\load_uni.c ^
%SRC_DIR%\loaders\load_xm.c ^
%SRC_DIR%\playercode\mdreg.c ^
%SRC_DIR%\playercode\mdriver.c ^
%SRC_DIR%\playercode\mloader.c ^
%SRC_DIR%\playercode\mlreg.c ^
%SRC_DIR%\playercode\mlutil.c ^
%SRC_DIR%\mmio\mmalloc.c ^
%SRC_DIR%\mmio\mmerror.c ^
%SRC_DIR%\mmio\mmio.c ^
%SRC_DIR%\depackers\mmcmp.c ^
%SRC_DIR%\depackers\pp20.c ^
%SRC_DIR%\depackers\s404.c ^
%SRC_DIR%\depackers\xpk.c ^
%SRC_DIR%\playercode\mplayer.c ^
%SRC_DIR%\playercode\munitrk.c ^
%SRC_DIR%\playercode\mwav.c ^
%SRC_DIR%\playercode\npertab.c ^
%SRC_DIR%\playercode\sloader.c ^
%SRC_DIR%\playercode\virtch.c ^
%SRC_DIR%\playercode\virtch2.c ^
%SRC_DIR%\playercode\virtch_common.c ^
%SRC_DIR%\posix\strcasecmp.c ^
..\..\src\drv_webaudio.c ^
..\..\src\main.c

REM General options: https://emscripten.org/docs/tools_reference/emcc.html
REM -s flags: https://github.com/emscripten-core/emscripten/blob/master/src/settings.js
REM
REM As of August 2020, WASM=2 does not work properly, even if loading the correct file
REM manually during runtime... That's why I was compiling it twice... But, according to
REM Can I Use, we can assume that if a browser supports AudioWorkletNode it also supports
REM WebAssembly (but not the other way around):
REM https://caniuse.com/wasm
REM https://caniuse.com/mdn-api_audioworkletnode

CALL emcc ^
	-I%INCLUDE_DIR% ^
	-s WASM=1 ^
	-s PRECISE_F32=0 ^
	-s DYNAMIC_EXECUTION=0 ^
	-s EXPORTED_FUNCTIONS="['_getVersion', '_init', '_freeModule', '_terminate', '_preLoadModule', '_loadModule', '_changeGeneralOptions', '_update', '_getErrno', '_getStrerr', '_getSongName', '_getModType', '_getComment', '_getAudioBuffer', '_getAudioBufferMaxLength', '_getAudioBufferUsedLength']" ^
	-s ALLOW_MEMORY_GROWTH=1 ^
	-s INITIAL_MEMORY=3145728 ^
	-s MAXIMUM_MEMORY=33554432 ^
	-s TOTAL_STACK=1048576 ^
	-s SUPPORT_LONGJMP=0 ^
	-s MINIMAL_RUNTIME=0 ^
	-s ASSERTIONS=0 ^
	-s STACK_OVERFLOW_CHECK=0 ^
	-s EXPORT_NAME=LibMikModCLib ^
	-s MODULARIZE=1 ^
	-s ENVIRONMENT='web,webview' ^
	-Os ^
	-DMIKMOD_UNIX=0 ^
	-DMIKMOD_BUILD ^
	-DHAVE_LIMITS_H ^
	-o %TEMP_DIR%\libmikmodclib.js ^
	%SRCS%

MOVE %TEMP_DIR%\libmikmodclib.wasm %LIB_DIR%\libmikmodclib.wasm
