@echo off
set VS=C:\Program Files (x86)\Microsoft Visual Studio 12.0
call "%VS%\VC\vcvarsall.bat" x86
cd scripts
@echo on
msbuild.exe Varibeat.sln /property:Configuration=Debug /m /t:Varibeat
pause