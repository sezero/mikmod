@ECHO OFF

CALL tscfdbg

REM ECMASCRIPT_2015 and ES6 are the same thing...
REM https://www.typescriptlang.org/docs/handbook/compiler-options.html (--target section)
REM https://github.com/google/closure-compiler/wiki/Flags-and-Options

REM We are using ECMASCRIPT_2015 (without async/await support) in favor of a few old Android devices... (here and at tsconfig.json)
java -jar D:\Tools\closure-compiler.jar --js dist\libmikmodaudionode.min.js --js_output_file dist\libmikmodaudionode.min.js --language_in ECMASCRIPT5 --language_out ECMASCRIPT5 --strict_mode_input --compilation_level SIMPLE
