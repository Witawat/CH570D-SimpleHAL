@echo off
rem ============================================================
rem clean.bat -- ลบไฟล์ build artifacts ทั้งหมด
rem ============================================================
echo === กำลังลบ build artifacts ===
if exist "obj" rmdir /s /q "obj"
if exist "output" rmdir /s /q "output"
echo เรียบร้อย
