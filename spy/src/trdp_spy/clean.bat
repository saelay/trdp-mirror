@echo off
rem SET WS_VERSION=1.8.0
SET WS_VERSION=1.8.3
rem set PATH=%PATH%;D:\Program Files\SlikSvn\bin;
set PATH=%PATH%;.
set PATH=%PATH%;c:\cygwin\bin
c:
cd "c:\Program Files\Microsoft Visual Studio 10.0\VC\"
rem %comspec% ""D:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"" x86
call "bin\vcvars32.bat"

c:
cd "c:\wireshark-%WS_VERSION%"

rem nmake -f Makefile.nmake verify_tools
rem nmake -f Makefile.nmake setup
rem nmake -f Makefile.nmake distclean
rem nmake -f Makefile.nmake all
rem nmake -f Makefile.nmake wireshark.bsc

cd plugins\trdp_spy
nmake -f Makefile.nmake distclean

del C:\wireshark-%WS_VERSION%\wireshark-gtk2\plugins\1.8.0-ForTRDPSpy\trdp_spy.dll
IF     ERRORLEVEL 1 goto :quit
goto:eof

:quit
echo ERROR..!!clean fail
