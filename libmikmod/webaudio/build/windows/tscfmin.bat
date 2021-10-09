@ECHO OFF

CALL tscfdbg

SET CLOSURE_COMPILER=D:\Tools\closure-compiler.jar
SET TS_DIR=..\..\ts
SET FRONTEND_DIR=%TS_DIR%\frontend
SET LIB_DIR=..\..\dist

REM ECMASCRIPT_2015 and ES6 are the same thing...
REM https://www.typescriptlang.org/docs/handbook/compiler-options.html (--target section)
REM https://github.com/google/closure-compiler/wiki/Flags-and-Options

REM We are using ECMASCRIPT_2015 (without async/await support) in favor of a few old Android devices... (here and at tsconfig.json)
java -jar %CLOSURE_COMPILER% --js %LIB_DIR%\libmikmod.min.js --js_output_file %FRONTEND_DIR%\temp\temp.js --language_in ECMASCRIPT5 --language_out ECMASCRIPT5 --strict_mode_input --emit_use_strict=false --compilation_level SIMPLE

TYPE %TS_DIR%\shared\header.js %FRONTEND_DIR%\temp\temp.js > %LIB_DIR%\libmikmod.min.js

DEL %FRONTEND_DIR%\temp\temp.js
