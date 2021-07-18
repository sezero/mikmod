@ECHO OFF

SET TS_DIR=..\..\ts
SET BACKEND_DIR=%TS_DIR%\backend
SET LIB_DIR=..\..\dist
SET SRC_DIR=..\..\src

CALL tsc --project %BACKEND_DIR%\tsconfig.json

DEL %BACKEND_DIR%\temp\backend\libmikmodclib.js
DEL %BACKEND_DIR%\temp\backend\missingtypes.js

TYPE %BACKEND_DIR%\temp\shared\shared.js %BACKEND_DIR%\temp\backend\libmikmod.js %BACKEND_DIR%\temp\backend\libmikmodprocessor.js %SRC_DIR%\temp\libmikmodclib.js > %LIB_DIR%\libmikmodprocessor.min.js

DEL %BACKEND_DIR%\temp\backend\libmikmod.js
DEL %BACKEND_DIR%\temp\backend\libmikmodprocessor.js
DEL %BACKEND_DIR%\temp\shared\shared.js
