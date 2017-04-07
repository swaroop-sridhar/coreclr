@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsDevCmd.bat"

goto Done

set ROOT=%cd%\LinkBench
set ExitCode=0
mkdir LinkBench 2> nul
pushd %ROOT%

echo Setting up  ** HelloWorld **
cd %ROOT%
git clone http://github.com/swaroop-sridhar/hello.git HelloWorld -b dev
cd HelloWorld
dotnet restore -r win10-x64
dotnet publish -c release -r win10-x64
dotnet msbuild /t:Link /p:LinkerMode=sdk /p:RuntimeIdentifier=win10-x64 /v:n /p:Configuration=release
if errorlevel 1 set ExitCode=1 
echo -- Done -- 

echo Setting up  ** MusicStore **
cd %ROOT%
git clone https://github.com/swaroop-sridhar/JitBench -b dev
cd JitBench\src\MusicStore
dotnet restore -r win10-x64 
dotnet publish -c release -r win10-x64
dotnet msbuild /t:Link /p:LinkerMode=sdk /p:RuntimeIdentifier=win10-x64 /v:n /p:LinkerRootFiles=MusicStoreReflection.xml /p:Configuration=release
if errorlevel 1 set ExitCode=1 
echo -- Done -- 

echo Setting up  ** MusicStore Ready2Run **
cd %ROOT%\JitBench\src\MusicStore
powershell -noprofile -executionPolicy RemoteSigned -file Get-Crossgen.ps1
pushd  bin\release\netcoreapp2.0\win10-x64\
call :SetupR2R publish_r2r
if errorlevel 1 set ExitCode=1 
call :SetupR2R linked_r2r
if errorlevel 1 set ExitCode=1 
echo -- Done -- 

echo Setting up ** CoreFX **
cd %ROOT%
git clone http://github.com/dotnet/corefx
cd corefx
set BinPlaceILLinkTrimAssembly=true
call build.cmd -release
if errorlevel 1 set ExitCode=1 
echo -- Done -- 

echo ** Roslyn **
cd %ROOT%
git clone https://github.com/swaroop-sridhar/roslyn.git -b dev
cd roslyn
set RoslynRoot=%cd%
REM Build Roslyn
call restore.cmd
msbuild /m Roslyn.sln  /p:Configuration=Release
REM Fetch ILLink
cd illink
dotnet restore link.csproj -r win10-x64 --packages bin
cd ..
REM Create Linker Directory
cd Binaries\Release\Exes
mkdir Linked
cd CscCore
REM Copy Unmanaged Assets
FOR /F "delims=" %%I IN ('DIR /b *') DO (
    corflags %%I >nul 2> nul
    if errorlevel 1 copy %%I ..\Linked >nul
)
copy *.ni.dll ..\Linked
REM Run Linker
dotnet %RoslynRoot%\illink\bin\illink.tasks\0.1.1-preview\tools\netcoreapp2.0\illink.dll -t -c link -a @%RoslynRoot%\RoslynRoots.txt -x %RoslynRoot%\RoslynRoots.xml -l none -out ..\Linked
if errorlevel 1 set ExitCode=1 
echo -- Done -- 
popd

:Done
exit /b %ExitCode%

:SetupR2R
REM Create R2R directory and copy all contents from MSIL to R2R directory
mkdir %1
xcopy /E /Y /Q publish %1
REM Generate Ready2Run images for all MSIL files by running crossgen
cd %1
copy ..\..\..\..\..\crossgen.exe 
FOR /F %%I IN ('dir /b *.dll ^| find /V /I ".ni.dll"  ^| find /V /I "System.Private.CoreLib" ^| find /V /I "mscorlib.dll"') DO (
    REM Don't crossgen Corlib, since the r2r images already exist.
    REM For all other MSIL files (corflags returns 0), run crossgen
    corflags %%I
    if not errorlevel 1 (
        echo crossgen /Platform_Assemblies_Paths . %%I
        crossgen /Platform_Assemblies_Paths . %%I
        if errorlevel 1 (
            exit /b 1
        )
    )
)
del crossgen.exe

REM Remove the original MSIL files, rename the Ready2Run files .ni.dll --> .dll
FOR /F "delims=" %%I IN ('dir /b *.dll') DO (
    if exist %%~nI.ni.dll (
        del %%I 
        ren %%~nI.ni.dll %%I
    )
)
cd ..
exit /b 0
