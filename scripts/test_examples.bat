@echo off
setlocal enabledelayedexpansion

rem ============================================================
rem test_examples.bat
rem
rem วนลูป build แต่ละตัวอย่างใน src/SimpleHAL/examples/
rem รายงาน PASS/FAIL และเก็บ output แยกตามชื่อ
rem
rem สิ่งที่ทำ:
rem   1. backup src/Main.c → src/Main.c.bak
rem   2. สำหรับแต่ละ example:
rem      - คัดลอก example/main.c → src/Main.c
rem      - set EXTRA_CFLAGS ถ้าเป็น BLE/RF
rem      - call scripts\build.bat
rem      - ถ้า PASS → copy output/* → output_test/<name>/
rem   3. restore src/Main.c
rem ============================================================

set EX_ROOT=src\SimpleHAL\examples
set MAIN_BAK=src\Main.c.bak
set OUTPUT_BASE=output_test

rem ---- backup ----
if exist "%MAIN_BAK%" del "%MAIN_BAK%"
copy /y src\Main.c "%MAIN_BAK%" >nul

set PASS=0
set FAIL=0

echo.
echo ========================================
echo  Starting example build tests
echo ========================================
echo.

for %%d in (
    00_blink_led 01_uart_echo 02_spi_lcd 03_i2c_sensor
    04_timer_blink 05_pwm_fade 06_adc_read 07_flash_uid
    08_rtc_alarm 09_power_sleep 10_rf_2g4 11_ble_peripheral
    12_keyscan 13_all_in_one 14_nonblock_delay 15_softimer_delay
    16_usb_host_kbd 17_usb_dev_kbd 18_cmp_threshold
) do (
    rem ---- คัดลอก source ----
    copy /y "%EX_ROOT%\%%d\main.c" "src\Main.c" >nul

    rem ---- กำหนด EXTRA_CFLAGS สำหรับ BLE/RF/USB ----
    set EXTRA_CFLAGS=
    if "%%d"=="10_rf_2g4" set EXTRA_CFLAGS=-DHAL_ENABLE_RF=1
    if "%%d"=="11_ble_peripheral" set EXTRA_CFLAGS=-DHAL_ENABLE_BLE=1
    if "%%d"=="16_usb_host_kbd" set EXTRA_CFLAGS=-DHAL_ENABLE_USBHOST=1
    if "%%d"=="17_usb_dev_kbd" set EXTRA_CFLAGS=-DHAL_ENABLE_USBDEV=1

    echo ===== Testing: %%d =====
    call scripts\build.bat

    if !ERRORLEVEL! == 0 (
        set /a PASS+=1
        if not exist "%OUTPUT_BASE%\%%d" mkdir "%OUTPUT_BASE%\%%d"
        copy /y output\* "%OUTPUT_BASE%\%%d\" >nul
        echo [PASS] %%d
    ) else (
        set /a FAIL+=1
        echo [FAIL] %%d
    )
    echo.
)

rem ---- restore ----
copy /y "%MAIN_BAK%" "src\Main.c" >nul
del "%MAIN_BAK%"

echo ========================================
echo  Results: PASS=%PASS%  FAIL=%FAIL%
echo ========================================

if %FAIL% gtr 0 exit /b 1
