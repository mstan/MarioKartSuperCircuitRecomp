@echo off
setlocal
cd /d "%~dp0"
set "GAME="
set "GAMES=0"
for %%G in (*Recomp.exe) do (
    set "GAME=%%~fG"
    set /a GAMES+=1 >nul
)
if not "%GAMES%"=="1" (
    echo Keep this script beside exactly one game executable.
    exit /b 2
)
echo The launcher will record diagnostic output.
echo Reproduce the problem and close the launcher normally when finished.
"%GAME%" --launcher > "%~dp0launcher-diagnostics.log" 2>&1
set "GAME_EXIT=%ERRORLEVEL%"
>> "%~dp0launcher-diagnostics.log" echo Exit code: %GAME_EXIT%
echo Report: launcher-diagnostics.log
exit /b %GAME_EXIT%
