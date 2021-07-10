@ECHO OFF

CALL tsc --project ts\frontend\tsconfig.json

MOVE dist\libmikmodaudionode.js dist\libmikmodaudionode.min.js
