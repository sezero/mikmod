@ECHO OFF

SET TS_DIR=..\..\ts
SET FRONTEND_DIR=%TS_DIR%\frontend
SET LIB_DIR=..\..\dist

CALL tsc --project %FRONTEND_DIR%\tsconfig.json

MOVE %LIB_DIR%\libmikmod.js %LIB_DIR%\libmikmod.min.js
