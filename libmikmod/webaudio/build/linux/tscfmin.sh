./tscfdbg.sh

CLOSURE_COMPILER=/d/Tools/closure-compiler.jar
TS_DIR=../../ts
FRONTEND_DIR=$TS_DIR/frontend
LIB_DIR=../../dist

# ECMASCRIPT_2015 and ES6 are the same thing...
# https://www.typescriptlang.org/docs/handbook/compiler-options.html (--target section)
# https://github.com/google/closure-compiler/wiki/Flags-and-Options

# We are using ECMASCRIPT_2015 (without async/await support) in favor of a few old Android devices... (here and at tsconfig.json)
java -jar $CLOSURE_COMPILER --js $LIB_DIR/libmikmod.min.js --js_output_file $FRONTEND_DIR/temp/temp.js --language_in ECMASCRIPT5 --language_out ECMASCRIPT5 --strict_mode_input --emit_use_strict=false --compilation_level SIMPLE

cat $TS_DIR/shared/header.js $FRONTEND_DIR/temp/temp.js > $LIB_DIR/libmikmod.min.js

rm $FRONTEND_DIR/temp/temp.js
