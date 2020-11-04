@echo off

if "%1"=="" goto :compilerDef
set _compiler=%1
goto :compilerEnd
:compilerDef
set _compiler="clang++"
:compilerEnd

if "%_compiler%"=="g++" goto :std2a
if "%_compiler%"=="gcc" goto :std2a
if "%2"=="c++2a" goto :std2a
if "%2"=="2a" goto :std2a
set _std="c++20"
goto :stdEnd
:std2a
set _std="c++2a"
:stdEnd

vmake %_compiler% -std=%_std% -o chatt.exe -lws2_32 -lntdll -luser32 -D_CRT_SECURE_NO_WARNINGS -mwindows NAMN