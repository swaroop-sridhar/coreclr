setlocal ENABLEDELAYEDEXPANSION 
@echo off

set EXITCODE=0
pushd %LinkBenchRoot%

if not exist %__dotnet% call :DotNet

if defined __test_HelloWorld call :HelloWorld
if defined __test_WebAPI call :WebAPI
if defined __test_MusicStore call :MusicStore
if defined __test_MusicStore_R2R call :MusicStore_R2R 
if defined __test_CoreFx call :CoreFx
if defined __test_Roslyn call :Roslyn

popd
exit /b %EXITCODE%

:DotNet
mkdir .Net
powershell -noprofile -executionPolicy RemoteSigned wget  https://raw.githubusercontent.com/dotnet/cli/master/scripts/obtain/dotnet-install.ps1 -OutFile dotnet-install.ps1
powershell -noprofile -executionPolicy RemoteSigned -file dotnet-install.ps1 -Channel master -InstallDir .Net
if not exist %__dotnet% set EXITCODE=1&& echo DotNet uninstalled
exit /b 

:HelloWorld
mkdir HelloWorld
cd HelloWorld
call %__dotnet% new console
if errorlevel 1 set EXITCODE=1
cd ..
exit /b 

:WebAPI
mkdir WebAPI
cd WebAPI
call %__dotnet% new webapi
if errorlevel 1 set EXITCODE=1
cd ..
exit /b

:MusicStore
git clone https://github.com/aspnet/JitBench -b dev
if errorlevel 1 set EXITCODE=1
exit /b

:MusicStore_R2R
REM MusicStore_R2R requires a previous MusicStore run.
REM No Additional Setup
exit /b

:CoreFx
git clone http://github.com/dotnet/corefx
if errorlevel 1 set EXITCODE=1
exit /b

:Roslyn
git clone https://github.com/dotnet/roslyn.git
if errorlevel 1 set EXITCODE=1
exit /b
