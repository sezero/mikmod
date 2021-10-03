TS_DIR=../../ts
FRONTEND_DIR=$TS_DIR/frontend
LIB_DIR=../../dist

tsc --project $FRONTEND_DIR/tsconfig.json

mv $LIB_DIR/libmikmod.js $LIB_DIR/libmikmod.min.js
