@echo off
REM ============================================
REM  Force_feedback_app - one-click rebuild & run
REM  Double-click this file: kill old app, compile, open new app
REM ============================================

REM Close running old app (otherwise exe is locked)
taskkill /F /IM Force_feedback_app.exe >nul 2>&1

REM Set Qt tool paths
set PATH=D:\Qt\5.14.2\mingw73_64\bin;D:\Qt\Tools\mingw730_64\bin;%PATH%

REM Enter project directory (%~dp0 = this file's folder)
cd /d "%~dp0"

echo === Compiling Force_feedback_app ... ===
qmake Force_feedback_app.pro
mingw32-make -j4

if errorlevel 1 (
    echo.
    echo *** BUILD FAILED *** please send me the error text above.
    pause
    exit /b 1
)

echo.
echo *** BUILD OK *** starting app...
start "" "release\Force_feedback_app.exe"
