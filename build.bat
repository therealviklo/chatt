@echo off

rem Konstanter
set _allFlags=-o chatt.exe -D_CRT_SECURE_NO_WARNINGS -Wno-unknown-warning-option
set _libs=-lws2_32 -lntdll -luser32 -lcomctl32

rem clang++ är defaultkompilatorn
if "%1"=="" ( set _compiler=clang++ ) else set _compiler=%1

rem GCC- eller Clangläge
set _mode=clang
if "%_compiler%"=="g++" set _mode=gcc
if "%_compiler%"=="gcc" set _mode=gcc

rem Början på loopen som går igenom argumenten
:loopStart
shift
if "%1"=="" goto :loopEnd

rem Kan manuellt sätta på GCC-läge
if not "%1"=="g++" if not "%1"=="gcc" goto :gccModeEnd
set _mode=gcc
:gccModeEnd

rem Kan sätta på striktare varningar
if "%1"=="-w" set _warningFlags= -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable

rem Extra argument till kompilatorn
if not "%1"=="-x" goto :extraFlagsEnd
shift
set _extraFlags=%_extraFlags% %1
:extraFlagsEnd

rem Slut på loopen
goto :loopStart
:loopEnd

rem Skriver ut info om kompileringen
echo Kompilator: %_compiler%
if "%_mode%"=="gcc" goto :modeGCC
set _flags=-std=c++20 -Wno-deprecated-anon-enum-enum-conversion
echo Läge: Clang
goto :modeEnd
:modeGCC
set _flags=-std=c++2a -mwindows -municode
echo Läge: GCC
:modeEnd
echo Kompilatorspecifika flaggor: %_flags%
echo Andra flaggor: %_allFlags%%_warningFlags%%_extraFlags%
echo Biblioteksflaggor: %_libs%

rem Kompilera resourcefilen
windres -F pe-x86-64 -o resources.o resources.rc
rem Kompilera programmet
vmake %_compiler% %_flags% %_allFlags% %_warningFlags%%_extraFlags% NAMN resources.o %_libs%