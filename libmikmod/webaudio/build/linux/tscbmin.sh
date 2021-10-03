./tscbdbg.sh

CLOSURE_COMPILER=/d/Tools/closure-compiler.jar
TS_DIR=../../ts
BACKEND_DIR=$TS_DIR/backend
LIB_DIR=../../dist

# ECMASCRIPT_2015 and ES6 are the same thing...
# https://www.typescriptlang.org/docs/handbook/compiler-options.html (--target section)
# https://github.com/google/closure-compiler/wiki/Flags-and-Options

# We are using ECMASCRIPT_2015 (without async/await support) in favor of a few old Android devices... (here and at tsconfig.json)
java -jar $CLOSURE_COMPILER --js $LIB_DIR/libmikmodprocessor.min.js --js_output_file $BACKEND_DIR/temp/temp.js --language_in ECMASCRIPT_2015 --language_out ECMASCRIPT_2015 --strict_mode_input --emit_use_strict=false --compilation_level SIMPLE

cat $TS_DIR/shared/header.js $BACKEND_DIR/temp/temp.js > $LIB_DIR/libmikmodprocessor.min.js

rm $BACKEND_DIR/temp/temp.js
