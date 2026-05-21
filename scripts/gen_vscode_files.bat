@echo off
setlocal enabledelayedexpansion

rem ============================================================
rem gen_vscode_files.bat -- สร้างไฟล์คอนฟิก .vscode/
rem ได้แก่ tasks.json, launch.json, c_cpp_properties.json
rem ============================================================

set "MRS=C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH"
set "VSDIR=%~dp0..\.vscode"
if not exist "%VSDIR%" mkdir "%VSDIR%"

rem ---- ตรวจหา GDB ----
if exist "%MRS%\Toolchain\RISC-V Embedded GCC\bin\riscv-none-embed-gdb.exe" (
    set "GDB=%MRS%\Toolchain\RISC-V Embedded GCC\bin\riscv-none-embed-gdb.exe"
) else if exist "%MRS%\Toolchain\RISC-V Embedded GCC12\bin\riscv-wch-elf-gdb.exe" (
    set "GDB=%MRS%\Toolchain\RISC-V Embedded GCC12\bin\riscv-wch-elf-gdb.exe"
) else (
    set "GDB=riscv-none-embed-gdb.exe"
)

rem ---- ตรวจหาคอมไพเลอร์สำหรับ c_cpp_properties ----
if exist "%MRS%\Toolchain\RISC-V Embedded GCC12\bin\riscv-wch-elf-gcc.exe" (
    set "CC=%MRS%\Toolchain\RISC-V Embedded GCC12\bin\riscv-wch-elf-gcc.exe"
) else if exist "%MRS%\Toolchain\RISC-V Embedded GCC\bin\riscv-none-embed-gcc.exe" (
    set "CC=%MRS%\Toolchain\RISC-V Embedded GCC\bin\riscv-none-embed-gcc.exe"
) else (
    set "CC=riscv-none-embed-gcc.exe"
)

set "GDB=%GDB:\=/%"
set "CC=%CC:\=/%"

echo พบ GDB: %GDB%
echo พบ CC:  %CC%

rem ---- สร้าง tasks.json ----
(
  echo {
  echo     "version": "2.0.0",
  echo     "tasks": [
  echo         {
  echo             "label": "Build CH570D",
  echo             "type": "shell",
  echo             "command": "cmd",
  echo             "args": [
  echo                 "/c",
  echo                 "scripts\\build.bat"
  echo             ],
  echo             "group": {
  echo                 "kind": "build",
  echo                 "isDefault": true
  echo             },
  echo             "problemMatcher": [],
  echo             "detail": "Build firmware (scripts\\build.bat)"
  echo         },
  echo         {
  echo             "label": "Clean CH570D",
  echo             "type": "shell",
  echo             "command": "cmd",
  echo             "args": [
  echo                 "/c",
  echo                 "scripts\\clean.bat"
  echo             ],
  echo             "problemMatcher": [],
  echo             "detail": "Remove build artifacts"
  echo         },
  echo         {
  echo             "label": "Rebuild CH570D",
  echo             "type": "shell",
  echo             "command": "cmd",
  echo             "args": [
  echo                 "/c",
  echo                 "scripts\\rebuild.bat"
  echo             ],
  echo             "problemMatcher": [],
  echo             "detail": "Clean then build"
  echo         },
  echo         {
  echo             "label": "Upload CH570D (WCH-Link)",
  echo             "type": "shell",
  echo             "command": "cmd",
  echo             "args": [
  echo                 "/c",
  echo                 "scripts\\upload.bat"
  echo             ],
  echo             "problemMatcher": [],
  echo             "detail": "Flash via OpenOCD (WCH-Link)"
  echo         }
  echo     ]
  echo }
) > "%VSDIR%\tasks.json"

rem ---- สร้าง launch.json ----
(
  echo {
  echo     "version": "0.2.0",
  echo     "configurations": [
  echo         {
  echo             "name": "Debug CH570D (OpenOCD)",
  echo             "type": "cppdbg",
  echo             "request": "launch",
  echo             "program": "${workspaceFolder}/obj/CH570D.elf",
  echo             "args": [],
  echo             "stopAtEntry": false,
  echo             "cwd": "${workspaceFolder}",
  echo             "environment": [],
  echo             "externalConsole": false,
  echo             "MIMode": "gdb",
  echo             "miDebuggerPath": "%GDB%",
  echo             "miDebuggerArgs": "",
  echo             "debugServerPath": "",
  echo             "debugServerArgs": "",
  echo             "serverStarted": "Listening on port",
  echo             "filterStderr": true,
  echo             "logging": {
  echo                 "moduleLoad": false,
  echo                 "trace": false,
  echo                 "engineLogging": false,
  echo                 "programOutput": true,
  echo                 "exceptions": false
  echo             },
  echo             "setupCommands": [
  echo                 {
  echo                     "description": "Enable pretty printing",
  echo                     "text": "-enable-pretty-printing",
  echo                     "ignoreFailures": true
  echo                 },
  echo                 {
  echo                     "description": "Set RISC-V architecture",
  echo                     "text": "set architecture riscv:rv32",
  echo                     "ignoreFailures": false
  echo                 },
  echo                 {
  echo                     "description": "Disable memory restrictions",
  echo                     "text": "set mem inaccessible-by-default off",
  echo                     "ignoreFailures": false
  echo                 },
  echo                 {
  echo                     "description": "Set disassembly flavor",
  echo                     "text": "set disassembler-options xw",
  echo                     "ignoreFailures": true
  echo                 }
  echo             ],
  echo             "preLaunchTask": "Build CH570D",
  echo             "presentation": {
  echo                 "group": "debug",
  echo                 "order": 1
  echo             }
  echo         }
  echo     ]
  echo }
) > "%VSDIR%\launch.json"

rem ---- สร้าง c_cpp_properties.json ----
(
  echo {
  echo     "configurations": [
  echo         {
  echo             "name": "CH570D",
  echo             "includePath": [
  echo                 "${workspaceFolder}/StdPeriphDriver/inc",
  echo                 "${workspaceFolder}/RVMSIS",
  echo                 "${workspaceFolder}/src",
  echo                 "${workspaceFolder}/src/SimpleHAL",
  echo                 "${workspaceFolder}/src/SimpleHAL/core"
  echo             ],
  echo             "defines": [
  echo                 "DEBUG=1"
  echo             ],
  echo             "intelliSenseMode": "linux-gcc-arm",
  echo             "compilerPath": "%CC%",
  echo             "cStandard": "gnu99",
  echo             "cppStandard": "gnu++11"
  echo         }
  echo     ],
  echo     "version": 4
  echo }
) > "%VSDIR%\c_cpp_properties.json"

echo เรียบร้อย ไฟล์ใน .vscode/:
echo   - tasks.json
echo   - launch.json
echo   - c_cpp_properties.json
echo.
