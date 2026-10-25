@echo off
setlocal
set "TOOLCHAIN=H:\KR-Compiler\KR-Compiler-Windows-x64-v4.3.1\toolchain"
set "COMPILER=%TOOLCHAIN%\bin\g++.exe"
if not exist "%COMPILER%" (
  echo KR-Compiler wurde nicht gefunden: %COMPILER%
  exit /b 1
)
set "PATH=%TOOLCHAIN%\bin;%PATH%"
"%COMPILER%" -std=c++17 -municode -mwindows -static -static-libgcc -static-libstdc++ src\main.cpp -lcomdlg32 -lshell32 -lole32 -o MockupCppConverter.exe
if errorlevel 1 exit /b 1
echo Fertig: %CD%\MockupCppConverter.exe
