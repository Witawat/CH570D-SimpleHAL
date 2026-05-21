@echo off
setlocal enabledelayedexpansion

rem ============================================================
rem build.bat -- คำสั่ง build เฟิร์มแวร์ CH570D
rem
rem รองรับ:
rem   TOOLCHAIN_CHOICE=1  -> GCC8  (riscv-none-embed-)
rem   TOOLCHAIN_CHOICE=2  -> GCC12 (riscv-wch-elf-)   [ค่าปกติ]
rem ============================================================

set MRS=C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH

if "%TOOLCHAIN_CHOICE%"=="" set TOOLCHAIN_CHOICE=2

rem ---- เลือก toolchain ตาม TOOLCHAIN_CHOICE ----
if "%TOOLCHAIN_CHOICE%"=="1" (
    set TOOLCHAIN_DIR=%MRS%\Toolchain\RISC-V Embedded GCC
    set CROSS_COMPILE=riscv-none-embed-
    set ARCH_FLAGS=-march=rv32imc -mabi=ilp32
) else (
    set TOOLCHAIN_DIR=%MRS%\Toolchain\RISC-V Embedded GCC12
    set CROSS_COMPILE=riscv-wch-elf-
    set ARCH_FLAGS=-march=rv32imc_xw -mabi=ilp32
)

set CC=%TOOLCHAIN_DIR%\bin\%CROSS_COMPILE%gcc.exe
set AS=%TOOLCHAIN_DIR%\bin\%CROSS_COMPILE%gcc.exe
set LD=%TOOLCHAIN_DIR%\bin\%CROSS_COMPILE%gcc.exe
set OBJCOPY=%TOOLCHAIN_DIR%\bin\%CROSS_COMPILE%objcopy.exe
set OBJDUMP=%TOOLCHAIN_DIR%\bin\%CROSS_COMPILE%objdump.exe
set SIZE=%TOOLCHAIN_DIR%\bin\%CROSS_COMPILE%size.exe

rem ---- ค่าตัวเลือกคอมไพล์ ----
set CFLAGS=%ARCH_FLAGS% -mcmodel=medany -Os -msave-restore
set CFLAGS=%CFLAGS% -ffunction-sections -fdata-sections -fno-common
set CFLAGS=%CFLAGS% -std=gnu99 -fsigned-char -fmessage-length=0
set CFLAGS=%CFLAGS% -DDEBUG=1
set CFLAGS=%CFLAGS% -I"StdPeriphDriver/inc" -I"RVMSIS" -I"src"
set CFLAGS=%CFLAGS% -I"src/SimpleHAL" -I"src/SimpleHAL/core"

rem ---- ค่าตัวเลือกลิงก์ ----
set LDFLAGS=%ARCH_FLAGS% -mcmodel=medany -nostartfiles
set LDFLAGS=%LDFLAGS% -T"Ld/Link.ld" -Wl,--gc-sections
set LDFLAGS=%LDFLAGS% --specs=nano.specs --specs=nosys.specs
set LDFLAGS=%LDFLAGS% -Wl,-Map=obj/CH570D.map

set PROJ_NAME=CH570D
set OBJ_DIR=obj
set OUT_DIR=output

if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo.
echo === Building %PROJ_NAME% ===
echo Toolchain: %TOOLCHAIN_DIR%
echo Arch: %ARCH_FLAGS%
echo.

set OBJ_FILES=

rem ---- คอมไพล์ซอร์สผู้ใช้ ----
echo [CC] src\Main.c
"%CC%" %CFLAGS% -c "src\Main.c" -o "%OBJ_DIR%\Main.o"
if errorlevel 1 exit /b 1
set OBJ_FILES=%OBJ_FILES% "%OBJ_DIR%\Main.o"

rem ---- คอมไพล์ SimpleHAL core ----
echo [CC] src\SimpleHAL\core\hal_ringbuf.c
"%CC%" %CFLAGS% -c "src\SimpleHAL\core\hal_ringbuf.c" -o "%OBJ_DIR%\hal_ringbuf.o"
if errorlevel 1 exit /b 1
set OBJ_FILES=%OBJ_FILES% "%OBJ_DIR%\hal_ringbuf.o"

echo [CC] src\SimpleHAL\core\hal_softimer.c
"%CC%" %CFLAGS% -c "src\SimpleHAL\core\hal_softimer.c" -o "%OBJ_DIR%\hal_softimer.o"
if errorlevel 1 exit /b 1
set OBJ_FILES=%OBJ_FILES% "%OBJ_DIR%\hal_softimer.o"

rem ---- คอมไพล์โมดูล SimpleHAL ----
for %%f in (
    hal_uart hal_gpio hal_spi hal_i2c hal_timer hal_pwm
    hal_adc hal_flash hal_rtc hal_pwr hal_clk hal_sys
    hal_keyscan
) do (
    echo [CC] src\SimpleHAL\%%f.c
    "%CC%" %CFLAGS% -c "src\SimpleHAL\%%f.c" -o "%OBJ_DIR%\%%f.o"
    if errorlevel 1 exit /b 1
    set OBJ_FILES=!OBJ_FILES! "%OBJ_DIR%\%%f.o"
)

rem ---- คอมไพล์ StdPeriphDriver (WCH SDK) ----
for %%f in (
    CH57x_clk CH57x_cmp CH57x_flash CH57x_gpio CH57x_i2c
    CH57x_keyscan CH57x_pwm CH57x_pwr CH57x_spi CH57x_sys
    CH57x_timer CH57x_uart CH57x_usbdev CH57x_usbhostBase CH57x_usbhostClass
) do (
    echo [CC] StdPeriphDriver\%%f.c
    "%CC%" %CFLAGS% -c "StdPeriphDriver\%%f.c" -o "%OBJ_DIR%\%%f.o"
    if errorlevel 1 exit /b 1
    set OBJ_FILES=!OBJ_FILES! "%OBJ_DIR%\%%f.o"
)

rem ---- ประกอบ startup assembly ----
echo [AS] Startup\startup_CH572.S
"%AS%" %ARCH_FLAGS% -c "Startup\startup_CH572.S" -o "%OBJ_DIR%\startup_CH572.o"
if errorlevel 1 exit /b 1
set OBJ_FILES=%OBJ_FILES% "%OBJ_DIR%\startup_CH572.o"

rem ---- ลิงก์ไฟล์ออบเจ็กต์ทั้งหมด ----
echo [LD] %OBJ_DIR%\%PROJ_NAME%.elf
"%LD%" %LDFLAGS% %OBJ_FILES% -L"StdPeriphDriver" -lISP572 -o "%OBJ_DIR%\%PROJ_NAME%.elf"
if errorlevel 1 exit /b 1

rem ---- หลังประมวลผล: สร้าง .hex และ .lst ----
echo [OBJCOPY] %OBJ_DIR%\%PROJ_NAME%.hex
"%OBJCOPY%" -O ihex "%OBJ_DIR%\%PROJ_NAME%.elf" "%OBJ_DIR%\%PROJ_NAME%.hex"

echo [OBJDUMP] %OBJ_DIR%\%PROJ_NAME%.lst
"%OBJDUMP%" -S -x -C -l -w "%OBJ_DIR%\%PROJ_NAME%.elf" > "%OBJ_DIR%\%PROJ_NAME%.lst"

echo [SIZE]
"%SIZE%" "%OBJ_DIR%\%PROJ_NAME%.elf"

rem ---- คัดลอกไฟล์ที่ต้องการไปยัง output/ ----
copy /y "%OBJ_DIR%\%PROJ_NAME%.elf" "%OUT_DIR%\%PROJ_NAME%.elf" >nul
copy /y "%OBJ_DIR%\%PROJ_NAME%.hex" "%OUT_DIR%\%PROJ_NAME%.hex" >nul
copy /y "%OBJ_DIR%\%PROJ_NAME%.lst" "%OUT_DIR%\%PROJ_NAME%.lst" >nul
copy /y "%OBJ_DIR%\%PROJ_NAME%.map" "%OUT_DIR%\%PROJ_NAME%.map" >nul

echo.
echo === Build complete: %OBJ_DIR%\%PROJ_NAME%.elf ===
echo Output files copied to %OUT_DIR%/
echo.
