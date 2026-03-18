@echo off
echo.
echo ===============================================
echo  Integrated Flow Control System - Windows 
echo ===============================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python not found!
    echo Please install Python 3.6+ from https://www.python.org/downloads/
    echo.
    pause
    exit /b 1
)

REM Run the quick start script
echo Starting system...
python start_system.py

echo.
echo System stopped.
pause