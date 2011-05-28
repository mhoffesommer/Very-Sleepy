@echo off

rem Visual studio build setup
rem =========================

if "%ProgramFiles(x86)%"=="" (
	set pf=%ProgramFiles%
) else (
	set pf=%ProgramFiles% (x86^)
)

echo This build script assumes that Visual Studio 2008 (9.0) is installed in 
echo %pf%\Microsoft Visual Studio 9.0
echo.
pause

call "%pf%\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat"

rem wxWindows
rem =========

cd wxwindows\build\msw

nmake -f makefile.vc BUILD=debug SHARED=0 RUNTIME_LIBS=static
nmake -f makefile.vc BUILD=release SHARED=0 RUNTIME_LIBS=static

cd ..\..\..

rem Sleepy
rem ======
cd sleepy

devenv sleepy.sln /build release 
devenv sleepy.sln /build debug

cd ..
pause
