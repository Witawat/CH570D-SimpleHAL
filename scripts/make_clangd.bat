@echo off
setlocal enabledelayedexpansion

rem ============================================================
rem make_clangd.bat -- สร้างไฟล์ .clangd สำหรับ clangd LSP
rem รองรับทั้ง GCC8 และ GCC12 toolchains
rem ============================================================

set "MRS=C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH"

rem ---- ตรวจหา toolchain (เลือก GCC12 ก่อน) ----
set "CC="
set "SYSINC1="

if exist "%MRS%\Toolchain\RISC-V Embedded GCC12\bin\riscv-wch-elf-gcc.exe" goto :use_gcc12
if exist "%MRS%\Toolchain\RISC-V Embedded GCC\bin\riscv-none-embed-gcc.exe" goto :use_gcc8
goto :not_found

:use_gcc12
set "TCDIR=%MRS%\Toolchain\RISC-V Embedded GCC12"
set "CC=%TCDIR%\bin\riscv-wch-elf-gcc.exe"
set "SYSINC1=%TCDIR%\riscv-wch-elf\include"
set "SYSINC2=%TCDIR%\lib\gcc\riscv-wch-elf\12.2.0\include"
set "SYSINC3=%TCDIR%\lib\gcc\riscv-wch-elf\12.2.0\include-fixed"
goto :generate

:use_gcc8
set "TCDIR=%MRS%\Toolchain\RISC-V Embedded GCC"
set "CC=%TCDIR%\bin\riscv-none-embed-gcc.exe"
set "SYSINC1=%TCDIR%\riscv-none-embed\include"
set "SYSINC2=%TCDIR%\lib\gcc\riscv-none-embed\8.2.0\include"
set "SYSINC3=%TCDIR%\lib\gcc\riscv-none-embed\8.2.0\include-fixed"
goto :generate

:not_found
echo ผิดพลาด: ไม่พบ RISC-V toolchain ภายใต้ %MRS%
exit /b 1

:generate
set "CC=%CC:\=/%"
set "SYSINC1=%SYSINC1:\=/%"
set "SYSINC2=%SYSINC2:\=/%"
set "SYSINC3=%SYSINC3:\=/%"

rem ---- project root (parent ของโฟลเดอร์ scripts) ----
pushd "%~dp0.."
set "PROJDIR=%CD%"
popd
set "PROJDIR=%PROJDIR:\=/%"

echo พบคอมไพเลอร์: %CC%

rem ---- เขียนไฟล์ .clangd ----
(
  echo # สร้างโดย scripts\make_clangd.bat
  echo # แก้ไขไฟล์นี้หรือรันสคริปต์อีกครั้งเพื่อสร้างใหม่
  echo.
  echo CompileFlags:
  echo   Compiler: "%CC%"
  echo   Add:
  echo     - "--target=riscv32-unknown-elf"
  echo     - "-x"
  echo     - "c"
  echo     - "-std=gnu99"
  echo     - "-DDEBUG=1"
  echo     - "-Wno-implicit-function-declaration"
  echo     - "-Wno-int-conversion"
  echo     - "-Wno-incompatible-pointer-types"
  echo     - "-Wno-attributes"
  echo     - "-I"
  echo     - "%PROJDIR%/StdPeriphDriver/inc"
  echo     - "-I"
  echo     - "%PROJDIR%/RVMSIS"
  echo     - "-I"
  echo     - "%PROJDIR%/src"
  echo     - "-I"
  echo     - "%PROJDIR%/src/SimpleHAL"
  echo     - "-I"
  echo     - "%PROJDIR%/src/SimpleHAL/core"
  echo     - "-I"
  echo     - "%SYSINC1%"
  echo     - "-I"
  echo     - "%SYSINC2%"
  echo     - "-I"
  echo     - "%SYSINC3%"
  echo   Remove:
  echo     - "-march=*"
  echo     - "-mabi=*"
  echo     - "-msave-restore"
  echo     - "-fmessage-length=*"
  echo     - "-fno-common"
  echo     - "-nostartfiles"
  echo     - "-Xlinker"
  echo     - "--gc-sections"
  echo     - "--specs=*"
  echo     - "-l*"
  echo     - "-L*"
  echo     - "-T*"
  echo     - "-Wl,*"
  echo.
  echo Diagnostics:
  echo   UnusedIncludes: None
  echo   Suppress:
  echo     - "pp_including_mainfile_in_preamble"
  echo   ClangTidy:
  echo     Remove:
  echo       - "modernize-*"
  echo       - "cppcoreguidelines-*"
  echo       - "readability-*"
  echo       - "performance-*"
  echo       - "bugprone-reserved-identifier"
  echo       - "cert-dcl37-c"
  echo       - "cert-dcl51-cpp"
  echo.
  echo Index:
  echo   Background: Build
) > "%~dp0..\.clangd"

echo เรียบร้อย เขียนไฟล์ ..\.clangd แล้ว
echo รีสตาร์ท clangd: Ctrl+Shift+P ^> clangd: Restart Language Server
echo.
