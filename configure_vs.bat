@echo off
setlocal EnableExtensions
cd /d "%~dp0"

where cmake >nul 2>&1
if errorlevel 1 (
  echo cmake was not found on PATH.
  exit /b 1
)

call :generator "%~1"

rem Prefer a current vcpkg. Visual Studio 2022's bundled scripts do not
rem implement REMOVE_DUPLICATES and fail while building expat.
set "VCPKG_DIR=..\vcpkg"
if defined VCPKG_ROOT if exist "%VCPKG_ROOT%\scripts\cmake\vcpkg_host_path_list.cmake" (
  findstr /C:"REMOVE_DUPLICATES" "%VCPKG_ROOT%\scripts\cmake\vcpkg_host_path_list.cmake" >nul
  if not errorlevel 1 set "VCPKG_DIR=%VCPKG_ROOT%"
)
if not exist "%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake" (
  echo Cloning vcpkg into %VCPKG_DIR%
  if not exist "%VCPKG_DIR%\.." mkdir "%VCPKG_DIR%\.."
  where git >nul 2>&1
  if errorlevel 1 (
    echo git was not found on PATH.
    exit /b 1
  )
  git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
  if errorlevel 1 exit /b 1
)
findstr /C:"REMOVE_DUPLICATES" "%VCPKG_DIR%\scripts\cmake\vcpkg_host_path_list.cmake" >nul
if errorlevel 1 (
  echo vcpkg at %VCPKG_DIR% does not implement REMOVE_DUPLICATES.
  echo Update it with: git -C "%VCPKG_DIR%" pull
  exit /b 1
)
if not exist "%VCPKG_DIR%\vcpkg.exe" (
  echo Bootstrapping vcpkg
  call "%VCPKG_DIR%\bootstrap-vcpkg.bat" -disableMetrics
  if errorlevel 1 exit /b 1
)

if exist "build\CMakeCache.txt" (
  findstr /I /C:"%VCPKG_DIR:\=/%/scripts/buildsystems/vcpkg.cmake" "build\CMakeCache.txt" >nul
  if errorlevel 1 (
    echo Removing build because it was configured with a different vcpkg.
    rmdir /s /q .\build
  )
)

echo Configuring PTLib with "%GENERATOR%"
echo vcpkg: %VCPKG_DIR%
cmake -S . -B .\build -G "%GENERATOR%" -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake" -DPTLIB_BUILD_SAMPLES=ON
if errorlevel 1 exit /b 1

echo.
if exist "%CD%\build\PTLib.slnx" (
  echo Created %CD%\build\PTLib.slnx
  echo Visual Studio 2026 opens this .slnx file. Visual Studio 2022 does not.
) else if exist "%CD%\build\PTLib.sln" (
  echo Created %CD%\build\PTLib.sln
) else (
  echo CMake finished but did not write build\PTLib.sln or build\PTLib.slnx.
  exit /b 1
)
exit /b 0

:generator
if /I "%~1"=="2017" set "GENERATOR=Visual Studio 15 2017" & exit /b 0
if /I "%~1"=="2019" set "GENERATOR=Visual Studio 16 2019" & exit /b 0
if /I "%~1"=="2022" set "GENERATOR=Visual Studio 17 2022" & exit /b 0
if /I "%~1"=="2026" set "GENERATOR=Visual Studio 18 2026" & exit /b 0
if not "%~1"=="" set "GENERATOR=%~1" & exit /b 0

rem installationVersion and catalog_productLineVersion are both 18 for the
rem newest Visual Studio. CMake's generator name is "Visual Studio 18 2026".
set "GENERATOR=Visual Studio 17 2022"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 0
set "VS_LINE="
for /f "usebackq tokens=1 delims=." %%V in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productLineVersion`) do set "VS_LINE=%%V"
if "%VS_LINE%"=="15" set "GENERATOR=Visual Studio 15 2017"
if "%VS_LINE%"=="16" set "GENERATOR=Visual Studio 16 2019"
if "%VS_LINE%"=="17" set "GENERATOR=Visual Studio 17 2022"
if "%VS_LINE%"=="18" set "GENERATOR=Visual Studio 18 2026"
exit /b 0
