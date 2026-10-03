@echo off
rem ============================================================
rem  Buckshot Roulette Ammo Counter - build script (MinGW-w64)
rem  Output: BuckshotCounter.exe  (static, stripped, tiny)
rem ============================================================
setlocal
cd /d "%~dp0"

set CC=C:\MinGW\mingw64\bin\gcc.exe
set RC=C:\MinGW\mingw64\bin\windres.exe
set OUT=BuckshotCounter.exe

if not exist "%CC%" goto noc
if not exist "%RC%" goto noc
if not exist build mkdir build

echo [1/4] build icon generator ...
"%CC%" -O2 -s make_icon.c -o build\make_icon.exe
if errorlevel 1 goto fail

echo [2/4] generate app.ico ...
pushd build
.\make_icon.exe
if errorlevel 1 goto failpop
popd
move /y build\app.ico app.ico >nul

echo [3/4] compile resources (icon / manifest / version) ...
"%RC%" --include-dir . app.rc -O coff -o build\app.res
if errorlevel 1 goto fail
if not exist build\app.res goto fail

echo [4/4] compile and link ...
"%CC%" -O2 -s -mwindows -fno-ident -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--nxcompat -Wl,--dynamicbase -Wl,--no-insert-timestamp buckshot_counter.c build\app.res -o "%OUT%" -lgdi32 -luser32 -lkernel32 -static
if errorlevel 1 goto fail

echo.
for %%A in ("%OUT%") do echo   OK: %%A  (%%~zA bytes)
echo.
exit /b 0

:noc
echo [ERROR] compiler or windres not found under C:\MinGW\mingw64\bin
exit /b 1
:failpop
popd
:fail
echo.
echo [ERROR] build failed
exit /b 1
