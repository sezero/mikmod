@ECHO OFF

CALL tscbdbg

SET CLOSURE_COMPILER=D:\Tools\closure-compiler.jar
SET TS_DIR=..\..\ts
SET BACKEND_DIR=%TS_DIR%\backend
SET LIB_DIR=..\..\dist

REM ECMASCRIPT_2015 and ES6 are the same thing...
REM https://www.typescriptlang.org/docs/handbook/compiler-options.html (--target section)
REM https://github.com/google/closure-compiler/wiki/Flags-and-Options

REM We are using ECMASCRIPT_2015 (without async/await support) in favor of a few old Android devices... (here and at tsconfig.json)
java -jar %CLOSURE_COMPILER% --js %LIB_DIR%\libmikmodprocessor.min.js --js_output_file %BACKEND_DIR%\temp\temp.js --language_in ECMASCRIPT_2015 --language_out ECMASCRIPT_2015 --strict_mode_input --emit_use_strict=false --compilation_level SIMPLE

TYPE %TS_DIR%\shared\header.js %BACKEND_DIR%\temp\temp.js > %LIB_DIR%\libmikmodprocessor.min.js

DEL %BACKEND_DIR%\temp\temp.js
