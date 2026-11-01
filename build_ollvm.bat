@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo === OLLVM Build Pipeline ===

set "ROOT=%~dp0"
set "OLLVM_DIR=%ROOT%ollvm"
set "PATCH_DIR=%ROOT%ollvm_patch"
set "BUILD_DIR=%ROOT%build_ollvm"
set "DLL_OUT=%ROOT%src\build\dxilconv.dll"

:: ---- find Visual Studio (needed for rc.exe, mspdb, Windows SDK) ----
:: NB: paths may contain "Program Files (x86)" — parenthesized blocks with
:: immediate %VAR% expansion break on such paths, so use goto-style flow.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_DIR="
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -products Microsoft.VisualStudio.Product.BuildTools,Microsoft.VisualStudio.Product.Community,Microsoft.VisualStudio.Product.Professional,Microsoft.VisualStudio.Product.Enterprise -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_DIR=%%i"
)
if not defined VS_DIR (
    echo ERROR: Visual Studio 2022 not found.
    echo Install VS 2022 Community with "Desktop development with C++"
    exit /b 1
)
echo VS: !VS_DIR!

set "VCVARS=!VS_DIR!\VC\Auxiliary\Build\vcvars64.bat"
if not exist "!VCVARS!" (
    echo ERROR: vcvars64.bat not found at !VCVARS!
    exit /b 1
)

:: ---- find ninja ----
where ninja >nul 2>&1
if %errorlevel% neq 0 (
    if exist "%ROOT%ninja.exe" (
        set "PATH=%ROOT%;%PATH%"
        echo ninja found in project root.
    ) else (
        echo ERROR: ninja not found.
        echo Put ninja.exe in %ROOT% or add to PATH
        exit /b 1
    )
)
for /f "tokens=*" %%i in ('where ninja') do echo Ninja: %%i

:: ---- find cmake ----
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    set "VS_CMAKE=!VS_DIR!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if exist "!VS_CMAKE!" (
        set "CMAKE_EXE=!VS_CMAKE!"
        echo cmake found in VS: !CMAKE_EXE!
    ) else (
        echo ERROR: cmake not in PATH.
        echo Run: set PATH=C:\cmake-3.30.2-windows-x86_64\bin;%%PATH%%
        exit /b 1
    )
) else (
    set "CMAKE_EXE=cmake"
)

:: ---- step 1: stock LLVM 16 ----
set "CLANG_CL=%OLLVM_DIR%\bin\clang-cl.exe"
set "LLD_LINK=%OLLVM_DIR%\bin\lld-link.exe"

if exist "%CLANG_CL%" goto llvm_ready

echo Downloading stock LLVM 16...
set "INSTALLER=%ROOT%LLVM-16.0.6-win64.exe"
if not exist "!INSTALLER!" (
    echo   curl -L -o "!INSTALLER!" "https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.6/LLVM-16.0.6-win64.exe"
    curl.exe -L -o "!INSTALLER!" "https://github.com/llvm/llvm-project/releases/download/llvmorg-16.0.6/LLVM-16.0.6-win64.exe"
    if !errorlevel! neq 0 (
        echo   Download failed. Get it manually from:
        echo   https://github.com/llvm/llvm-project/releases/tag/llvmorg-16.0.6
        echo   Save as: !INSTALLER!
        exit /b 1
    )
)
echo   Installing...
"!INSTALLER!" /S /D=%OLLVM_DIR:\=/%
del "!INSTALLER!" 2>nul
if not exist "%CLANG_CL%" (
    echo   LLVM install failed
    exit /b 1
)
echo   LLVM 16 installed.

:llvm_ready

:: ---- step 2: OLLVM patched clang-cl.exe ----
set "PATCHED_EXE=%PATCH_DIR%\clang-cl.exe"
set "BACKUP_CLANG=%OLLVM_DIR%\bin\clang-cl.exe.orig"

if not exist "%PATCH_DIR%" mkdir "%PATCH_DIR%"

if not exist "%PATCHED_EXE%" (
    echo clang-cl.exe not found in ollvm_patch\
    echo Download clang-cl.7z from https://github.com/wwh1004/ollvm-16/releases
    echo Extract clang-cl.exe into ollvm_patch\
    exit /b 1
)

if not exist "%BACKUP_CLANG%" copy /y "%CLANG_CL%" "%BACKUP_CLANG%" >nul
copy /y "%PATCHED_EXE%" "%CLANG_CL%" >nul
echo OLLVM clang-cl.exe overlayed.

:: ---- step 3: setup VS environment ----
:: Ninja + clang-cl needs VS toolchain for rc.exe, includes, libs, mspdb
echo Setting up VS environment...
call "!VCVARS!" >nul

:: ---- step 4: build ----
echo Cleaning build dir...
rmdir /s /q "%BUILD_DIR%" 2>nul
mkdir "%BUILD_DIR%"

echo Configuring CMake with Ninja + clang-cl...

cmake.exe -B "%BUILD_DIR%" -S "%ROOT%src" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DHELLO_USE_OLLVM=ON ^
    -DCMAKE_C_COMPILER="%CLANG_CL:\=/%" ^
    -DCMAKE_CXX_COMPILER="%CLANG_CL:\=/%" ^
    -DCMAKE_LINKER="%LLD_LINK:\=/%" ^
    -G Ninja

if %errorlevel% neq 0 (
    echo CMake configure failed.
    exit /b 1
)

echo Building with Ninja...
cmake.exe --build "%BUILD_DIR%" --config Release
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b 1
)

if exist "%DLL_OUT%" (
    for %%A in ("%DLL_OUT%") do echo DLL size: %%~zA bytes
    echo.
    echo === SUCCESS ===
    echo DLL: %DLL_OUT%
    echo OLLVM passes: fla split sub bcf sobf mba
    echo.
    echo Inject: Process Hacker -^> GTA5.exe -^> Misc -^> Inject DLL
) else (
    echo DLL not found at %DLL_OUT%
    echo Check output above for errors.
)
