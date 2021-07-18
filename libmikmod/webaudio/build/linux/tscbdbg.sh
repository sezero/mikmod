TS_DIR=../../ts
BACKEND_DIR=$TS_DIR/backend
LIB_DIR=../../dist
SRC_DIR=../../src

tsc --project $BACKEND_DIR/tsconfig.json

rm $BACKEND_DIR/temp/backend/libmikmodclib.js
rm $BACKEND_DIR/temp/backend/missingtypes.js

cat $BACKEND_DIR/temp/shared/shared.js $BACKEND_DIR/temp/backend/libmikmod.js $BACKEND_DIR/temp/backend/libmikmodprocessor.js $SRC_DIR/temp/libmikmodclib.js > $LIB_DIR/libmikmodprocessor.min.js

rm $BACKEND_DIR/temp/backend/libmikmod.js
rm $BACKEND_DIR/temp/backend/libmikmodprocessor.js
rm $BACKEND_DIR/temp/shared/shared.js
