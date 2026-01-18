@echo off
setlocal enabledelayedexpansion

REM ============================================================================
REM CyNickal Software EFT Build Script
REM ============================================================================
REM This script builds the EFT_DMA project using MSBuild
REM
REM Usage: build.bat [Configuration] [Platform]
REM   Configuration: Release (default), Debug, or DLL
REM   Platform: x64 (default) or Win32
REM ============================================================================

set "ROOT=%~dp0"
set "ROOT_NO_TRAIL=%ROOT:~0,-1%"
set "SOLUTIONDIR=%ROOT_NO_TRAIL%\\"
pushd "%ROOT%"


REM Set default configuration and platform
set "CONFIG=%~1"
set "PLATFORM=%~2"
if "%CONFIG%"=="" set "CONFIG=Release"
if "%PLATFORM%"=="" set "PLATFORM=x64"

REM Normalize platform input
if /I "%PLATFORM%"=="AnyCPU" set "PLATFORM=x64"
if /I "%PLATFORM%"=="Any CPU" set "PLATFORM=x64"
if /I "%PLATFORM%"=="x86" set "PLATFORM=Win32"

set "ARCH=x64"
if /I "%PLATFORM%"=="Win32" set "ARCH=x86"

REM Locate Visual Studio installation using vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do set "VSINSTALLPATH=%%i"
)

if not defined VSINSTALLPATH (
    if defined VSINSTALLDIR set "VSINSTALLPATH=%VSINSTALLDIR%"
)

if not defined VSINSTALLPATH (
    REM Fallback to common VS paths
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community" (
        set "VSINSTALLPATH=C:\Program Files\Microsoft Visual Studio\18\Community"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community" (
        set "VSINSTALLPATH=C:\Program Files\Microsoft Visual Studio\2022\Community"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional" (
        set "VSINSTALLPATH=C:\Program Files\Microsoft Visual Studio\2022\Professional"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise" (
        set "VSINSTALLPATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
    )
)

if not defined VSINSTALLPATH (
    echo Error: Could not find Visual Studio installation.
    echo Please ensure Visual Studio 2022 or later is installed.
    popd
    exit /b 1
)

set "VSDEVCMD=%VSINSTALLPATH%\Common7\Tools\VsDevCmd.bat"
set "VCVARS=%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat"

if exist "%VSDEVCMD%" (
    call "%VSDEVCMD%" -arch=%ARCH%
) else if exist "%VCVARS%" (
    call "%VCVARS%" %ARCH%
) else (
    echo Error: Could not find VsDevCmd.bat or vcvarsall.bat.
    echo Check the Visual Studio installation.
    popd
    exit /b 1
)

REM Set MSBuild path
set "MSBUILD=%VSINSTALLPATH%\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" set "MSBUILD=%VSINSTALLPATH%\MSBuild\15.0\Bin\MSBuild.exe"

REM Check if MSBuild exists
if not exist "%MSBUILD%" (
    echo Error: MSBuild.exe not found at: %MSBUILD%
    popd
    exit /b 1
)

set "PROJECT=CyNickal Software EFT\CyNickal Software EFT.vcxproj"

echo ============================================================================
echo Building CyNickal Software EFT
echo ============================================================================
echo Visual Studio Path: %VSINSTALLPATH%
echo MSBuild Path: %MSBUILD%
echo Project: %PROJECT%
echo Configuration: %CONFIG%
echo Platform: %PLATFORM%
echo ============================================================================

REM Build the project
echo.
echo Building project...
"%MSBUILD%" "%PROJECT%" /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% "/p:SolutionDir=%SOLUTIONDIR%" /t:Build /m /v:minimal


if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================================================
    echo BUILD FAILED with error code %ERRORLEVEL%
    echo ============================================================================
    popd
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================================================
echo BUILD SUCCEEDED
echo Output: %PLATFORM%\%CONFIG%\EFT_DMA.exe
echo ============================================================================

popd
exit /b 0
