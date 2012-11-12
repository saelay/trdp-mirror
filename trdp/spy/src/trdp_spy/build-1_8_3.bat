@echo off
SET WS_VERSION=1.8.3
rem set PATH=%PATH%;D:\Program Files\SlikSvn\bin;
set PATH=%PATH%;.
set PATH=%PATH%;c:\cygwin\bin
c:
cd "C:\Program Files\Microsoft Visual Studio 10.0\VC\"
call "bin\vcvars32.bat"

c:
cd "c:\wireshark-%WS_VERSION%"
set http_proxy=webproxy.apac.bombardier.com:8080

rem mkdir %WS_VERSION%
rem cd %WS_VERSION%
rem svn co http://anonsvn.wireshark.org/wireshark/releases/wireshark-1.8.0/

rem ------------------------  modify the fileending for windows
cd tools
dos2unix win32-setup.sh
dos2unix textify.sh
cd "c:\wireshark-%WS_VERSION%"

rem nmake -f Makefile.nmake verify_tools
rem nmake -f Makefile.nmake setup
rem nmake -f Makefile.nmake distclean
nmake -f Makefile.nmake all
rem nmake -f Makefile.nmake wireshark.bsc

cd plugins\trdp_spy
nmake -f Makefile.nmake distclean
nmake -f Makefile.nmake all



IF     ERRORLEVEL 1 goto :quit

copy trdp_spy.dll C:\wireshark-%WS_VERSION%\wireshark-gtk2\plugins\1.8.3
copy C:\wireshark-%WS_VERSION%\wireshark-gtk2\libxml2-2.dll C:\wireshark-%WS_VERSION%\wireshark-gtk2\libxml2.dll

goto:eof

cd ..
cd ..
nmake -f makefile.nmake packaging

:quit
echo ERROR..!!build fail

