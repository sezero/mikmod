@ECHO OFF

CALL tsc --project ts\backend\tsconfig.json

DEL ts\backend\temp\backend\libmikmodclib.js
DEL ts\backend\temp\backend\missingtypes.js

TYPE ts\backend\temp\shared\shared.js ts\backend\temp\backend\libmikmod.js ts\backend\temp\backend\libmikmodprocessor.js src\temp\libmikmodclib.js > dist\libmikmodprocessor.min.js

DEL ts\backend\temp\backend\libmikmod.js
DEL ts\backend\temp\backend\libmikmodprocessor.js
DEL ts\backend\temp\shared\shared.js
