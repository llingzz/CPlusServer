color 0A && echo off

rem protoc程序名
set "PROTOC_EXE=protoc.exe"
rem .proto文件名
set "PROTOCOL_FILE_NAME=protocol.proto"

%PROTOC_EXE% --version

set "WORK_DIR=%cd%"
rem cpp
set "CPP_OUT_PATH=%cd%"
if not exist %CPP_OUT_PATH% md %CPP_OUT_PATH%
rem cs
rem set "CS_OUT_PATH=%cd%\CSharp"
rem if not exist %CS_OUT_PATH% md %CS_OUT_PATH%
rem js
rem set "JS_OUT_PATH=%cd%\JavaScripts"
rem if not exist %JS_OUT_PATH% md %JS_OUT_PATH%
rem python
rem set "PYTHON_OUT_PATH=%cd%\Python"
rem if not exist %PYTHON_OUT_PATH% md %PYTHON_OUT_PATH%

echo.generate cpp
"%WORK_DIR%\%PROTOC_EXE%" --proto_path="%WORK_DIR%" --cpp_out="%CPP_OUT_PATH%" "%WORK_DIR%\%PROTOCOL_FILE_NAME%"
rem echo.generate cs
rem "%WORK_DIR%\%PROTOC_EXE%" --proto_path="%WORK_DIR%" --csharp_out="%CS_OUT_PATH%" "%WORK_DIR%\%PROTOCOL_FILE_NAME%"
rem echo.generate js
rem "%WORK_DIR%\%PROTOC_EXE%" --proto_path="%WORK_DIR%" --js_out="%JS_OUT_PATH%" "%WORK_DIR%\%PROTOCOL_FILE_NAME%"
rem echo.generate python
rem "%WORK_DIR%\%PROTOC_EXE%" --proto_path="%WORK_DIR%" --python_out="%PYTHON_OUT_PATH%" "%WORK_DIR%\%PROTOCOL_FILE_NAME%"
pause