@echo off

if not exist "build" (
    mkdir "build"
    echo Directory created at build
)

if not exist "build\genie.exe" (
    echo Downloading genie...
    rem bitsadmin /transfer "genie" "https://github.com/bkaradzic/bx/raw/master/tools/bin/windows/genie.exe" "%~dp0genie.exe"
    powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://github.com/bkaradzic/bx/raw/master/tools/bin/windows/genie.exe' -OutFile '%~dp0build\genie.exe'"
)

IF "%1"=="" (
    echo No argument set
    echo Example: build.bat vs2015
    goto :eof
)

build\genie.exe %*