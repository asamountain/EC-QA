@echo off
REM ============================================================================
REM BOQU IOT-485-EC4A Automated Test Runner
REM One-Click Solution for QA Intern Testing
REM ============================================================================

setlocal enabledelayedexpansion

echo.
echo ============================================================================
echo   BOQU Sensor Auto-Test Launcher
echo ============================================================================
echo.

REM Step 1: Check if running as Administrator
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] This script must run as Administrator!
    echo Right-click run_test.bat and select "Run as administrator"
    echo.
    pause
    exit /b 1
)

echo [1/5] Checking for usbipd installation...
where usbipd >nul 2>&1
if %errorLevel% neq 0 (
    echo [ERROR] usbipd-win is not installed!
    echo Please install it first: winget install usbipd
    echo.
    pause
    exit /b 1
)
echo      OK - usbipd found

echo.
echo [2/5] Scanning for USB-SERIAL CH340 device...

REM Get the list and find the CH340 device
set BUSID=
set DEVICE_FOUND=0

for /f "tokens=1,* delims= " %%a in ('usbipd list ^| findstr /i "CH340"') do (
    set BUSID=%%a
    set DEVICE_FOUND=1
    echo      Found: %%a %%b
)

if !DEVICE_FOUND!==0 (
    echo [ERROR] USB-SERIAL CH340 not found!
    echo.
    echo Please check:
    echo   1. USB adapter is plugged in
    echo   2. Device appears in Device Manager
    echo.
    usbipd list
    echo.
    pause
    exit /b 1
)

echo      BUSID: !BUSID!

echo.
echo [3/5] Checking device state...

REM Check if device is already attached
usbipd list | findstr /i "!BUSID!" | findstr /i "Attached" >nul 2>&1
if %errorLevel% equ 0 (
    echo      Device already attached to WSL
    goto :run_program
)

REM Check if device is shared (bound)
usbipd list | findstr /i "!BUSID!" | findstr /i "Shared" >nul 2>&1
if %errorLevel% neq 0 (
    echo      Device not bound. Binding now...
    usbipd bind --busid !BUSID!
    if %errorLevel% neq 0 (
        echo [ERROR] Failed to bind device
        pause
        exit /b 1
    )
    echo      Successfully bound device
)

echo.
echo [4/5] Attaching device to WSL...
usbipd attach --wsl --busid !BUSID!
if %errorLevel% neq 0 (
    echo [ERROR] Failed to attach device to WSL
    echo.
    echo Troubleshooting:
    echo   1. Make sure WSL is running: wsl --status
    echo   2. Try manually: usbipd attach --wsl --busid !BUSID!
    echo.
    pause
    exit /b 1
)

echo      Device successfully attached!

REM Wait for device to initialize in WSL
timeout /t 2 /nobreak >nul

:run_program
echo.
echo [5/5] Launching Smart Logger in WSL...
echo ============================================================================
echo.

REM Change to the correct directory and run the program
wsl -e bash -c "cd '/mnt/c/Users/iocrops admin/Coding/EC-QA' && if [ ! -f smart_logger ]; then echo 'ERROR: smart_logger not compiled! Run: g++ -o smart_logger smart_logger.cpp \$(pkg-config --cflags --libs libmodbus)'; exit 1; fi && sudo ./smart_logger"

REM After program exits
echo.
echo ============================================================================
echo   Test Session Ended
echo ============================================================================
echo.
echo Data saved to: ec_data_log.csv
echo.
echo Next steps:
echo   1. Run visualization: python3 plot_data.py
echo   2. Or restart test: run_test.bat
echo.

REM Ask if user wants to detach the device
set /p DETACH="Detach USB device from WSL? (Y/N): "
if /i "!DETACH!"=="Y" (
    echo Detaching device...
    usbipd detach --busid !BUSID!
    echo Device detached. Available for Windows use.
)

echo.
pause
