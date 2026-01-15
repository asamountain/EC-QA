@echo off
REM ============================================================================
REM BOQU IOT-485-EC4A Automated Test Runner
REM One-Click Solution for QA Intern Testing
REM ============================================================================
REM FEATURES:
REM   - Auto-elevates to Administrator (triggers UAC prompt automatically)
REM   - Auto-detects USB-SERIAL CH340 device via usbipd
REM   - Binds and attaches device to WSL2
REM   - Compiles and launches smart_logger in WSL
REM ============================================================================

setlocal enabledelayedexpansion

REM ============================================================================
REM STEP 0: SELF-ELEVATION HACK (Auto-Admin)
REM ============================================================================
REM Check if we already have admin privileges using "net session"
REM If not, re-launch this script as Administrator via PowerShell

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo.
    echo ============================================================================
    echo   Requesting Administrator Privileges...
    echo ============================================================================
    echo.
    echo This script requires Administrator access for USB passthrough.
    echo A UAC prompt will appear - please click "Yes" to continue.
    echo.
    
    REM Use PowerShell to re-launch this batch file with elevation
    REM -Verb RunAs triggers the UAC elevation prompt
    REM %~dpnx0 expands to the full path of this batch file
    REM We use Start-Process with -Wait so the elevated window stays open
    
    powershell -Command "Start-Process -FilePath '%~dpnx0' -Verb RunAs -Wait"
    
    REM Exit the non-elevated instance
    exit /b 0
)

REM If we reach here, we have admin privileges
echo.
echo ============================================================================
echo   BOQU Sensor Auto-Test Launcher [ADMINISTRATOR]
echo ============================================================================
echo.

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
    usbipd bind --busid !BUSID! --force
    if %errorLevel% neq 0 (
        echo [WARNING] Bind returned error, but may still work. Continuing...
    ) else (
        echo      Successfully bound device
    )
    REM Small delay to let the bind take effect
    timeout /t 1 /nobreak >nul
) else (
    echo      Device already bound/shared
)

echo.
echo [4/5] Attaching device to WSL...

REM Ensure WSL is running before attaching
echo      Waking up WSL...
wsl -e echo "WSL Ready" >nul 2>&1
timeout /t 1 /nobreak >nul

REM Use START /MIN for non-blocking attach (prevents script from hanging)
REM The /B flag runs without creating a new window, /MIN minimizes if window is created
echo      Starting USB attach (non-blocking)...
start "USB_Link" /MIN /B usbipd attach --wsl --busid !BUSID!

REM Give the attach command time to complete
echo      Waiting for USB link to establish...
timeout /t 4 /nobreak >nul

REM Verify attachment status
usbipd list | findstr /i "!BUSID!" | findstr /i "Attached" >nul 2>&1
if %errorLevel% neq 0 (
    echo [WARNING] Device may not be fully attached yet.
    echo           Will attempt to run anyway...
    timeout /t 2 /nobreak >nul
) else (
    echo      Device successfully attached!
)

REM Wait for device to initialize in WSL (serial port needs time to appear)
echo      Waiting for serial port to initialize...
timeout /t 3 /nobreak >nul

:run_program
echo.
echo [5/5] Launching Smart Logger in WSL...
echo ============================================================================
echo.
echo NOTE: If prompted for password, enter your WSL sudo password.
echo       Press Ctrl+C to stop logging.
echo.
echo ============================================================================
echo  If the program is missing or outdated, compile manually in WSL:
echo.
echo    g++ smart_logger.cpp -o smart_logger -I/usr/include/modbus -lmodbus
echo.
echo ============================================================================
echo.

REM Simple, safe execution - just run the pre-compiled binary
wsl bash -c "cd /mnt/c/Users/user/Coding/ATCAdjustedECValuation && sudo ./smart_logger"

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
