@echo off
setlocal

echo Started
:: Check if the argument is provided
if "%~1"=="" (
    echo Usage: %0 assets\fs_cubes.sc
    exit /b 1
)

:: Set the input file
set INPUT_FILE=%~1

set TYPE=%~2

:: Extract the filename without extension
set FILENAME=%~n1

echo %cd%

:: Create the necessary directories if they don't exist
if not exist "..\..\bin\shaders\dx11\" (
    mkdir "..\..\bin\shaders\dx11\"
)
echo made dir

echo ..\..\win64_vs2022\bin\shadercDebug.exe -f %INPUT_FILE% -o ..\..\bin\shaders\dx11\%FILENAME%.bin -i ..\..\..\bgfx\src\ -i ..\..\..\assets --platform windows --type %TYPE% -p s_5_0 -O 3

:: Run the shader compilation command with dynamic output filename
..\..\win64_vs2022\bin\shadercDebug.exe -f %INPUT_FILE% -o ..\..\bin\shaders\dx11\%FILENAME%.bin -i ..\..\..\3rdparty\bgfx\src\ -i ..\..\..\assets --platform windows --type %TYPE% -p s_5_0 -O 3

:: End of script
endlocal