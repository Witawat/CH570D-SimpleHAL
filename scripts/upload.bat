@echo off
setlocal

rem ============================================================
rem upload.bat -- อัปโหลดเฟิร์มแวร์ไปยัง CH570D ผ่าน WCH-Link
rem ============================================================

set MRS=C:\MounRiver\MounRiver_Studio2\resources\app\resources\win32\components\WCH

set OPENOCD=%MRS%\OpenOCD\OpenOCD\bin\openocd.exe
set OPENOCD_CFG=%MRS%\OpenOCD\OpenOCD\bin\wch-riscv.cfg

if not exist "%OPENOCD%" (
    echo ผิดพลาด: หา OpenOCD ไม่พบที่ "%OPENOCD%"
    exit /b 1
)

rem ---- ตรวจหาไฟล์ .hex (output/ ก่อน, ถ้าไม่เจอใช้ obj/) ----
if exist "output\CH570D.hex" goto :flash_output
if exist "obj\CH570D.hex" goto :flash_obj
echo ผิดพลาด: ไม่พบ CH570D.hex ให้รัน build.bat ก่อน
exit /b 1

:flash_output
set IMG_DIR=output
goto :do_flash

:flash_obj
set IMG_DIR=obj
goto :do_flash

:do_flash
echo === กำลังอัปโหลด CH570D ===
echo OpenOCD: %OPENOCD%
echo Config:  %OPENOCD_CFG%
echo Image:   %IMG_DIR%\CH570D.hex
echo.

"%OPENOCD%" -f "%OPENOCD_CFG%" -c "program %IMG_DIR%/CH570D.hex verify reset exit"

if errorlevel 1 (
    echo.
    echo อัปโหลดล้มเหลว!
    exit /b 1
) else (
    echo.
    echo อัปโหลดเสร็จสมบูรณ์
)
