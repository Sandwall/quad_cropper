@echo off
setlocal
cd /D "%~dp0"

if not exist "src/main.cpp" (
	echo Please run this script from the root directory of the project!
	exit /b
)

:: Check arguments
set release=0
if "%1" == "release" set release=1

:: Prepare flags & output directory
set link_flags=
set mode=Debug
set programName=quad_cropper
set programFolder=.\bin\%mode%\
set objectFolder=.\obj\%mode%\
set programPath=%programFolder%\%programName%

if %release% == 0 (
	set cl_flags=/Od /D "_DEBUG" /MDd /RTC1
	set link_flags=%link_flags% /PDB:%programPath%.pdb /DEBUG /INCREMENTAL /ILK:%programPath%.ilk /SUBSYSTEM:CONSOLE
) else (
	set mode=Release
	set programFolder=.\bin\%mode%\
	set objectFolder=.\obj\%mode%\
	set programPath=%programFolder%\%programName%
	set cl_flags=/O2 /GL /D "NDEBUG" /MD /Oi
	set link_flags=%link_flags% /INCREMENTAL:NO /SUBSYSTEM:WINDOWS
)

set cl_flags=%cl_flags% /std:c++20 /EHsc /GS /W3 /sdl /Zc:wchar_t /Zi /Zc:inline /Zc:forScope /nologo /FC /Gd /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /nologo /Fd%programFolder%
set link_flags=%link_flags% /DYNAMICBASE /MANIFEST /NXCOMPAT /LIBPATH:.\lib /MACHINE:x64 /NOLOGO /OUT:%programPath%.exe

:: Create folders if they don't exist
if not exist "./bin/" mkdir bin
pushd bin
if not exist %mode% mkdir %mode%
popd
if not exist "./obj/" mkdir obj
pushd obj
if not exist %mode% mkdir %mode%
popd

:: Perform compilation
if not exist %objectFolder%libs.obj cl /I.\include /c src/libs.cpp /Fo%objectFolder%libs.obj %cl_flags%

cl /I.\include /c src/main.cpp /Fo%objectFolder%main.obj %cl_flags% 
link %objectFolder%main.obj %objectFolder%libs.obj %link_flags%

echo Build complete!
