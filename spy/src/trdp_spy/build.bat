@echo off
SET WS_VERSION=1.8.3

set PATH=%PATH%;.
set PATH=%PATH%;c:\cygwin\bin
SET WORKINGDIR=%~dp0
SET WORKINGDISK=%CD:~0,2%
SET RESOURCES_DIR=..\..\resources\windows
SET LIBXML=libxml
SET LIBXML_DIR=%RESOURCES_DIR%\%LIBXML%
SET ICONV=iconv-1.9.2.win32
SET ICONV_DIR=%RESOURCES_DIR%\%ICONV%
SET SVN_OUTPUT=%WORKINGDIR%\..\..\win32

echo ====================== TRDP-SPY setup ======================
rem copy the plugin to the place where it is needed to be when compiling
mkdir c:\wireshark-%WS_VERSION%\plugins\trdp_spy\
mkdir c:\wireshark-%WS_VERSION%\plugins\trdp_spy\%LIBXML%
mkdir c:\wireshark-%WS_VERSION%\plugins\trdp_spy\%ICONV%
cd ..
copy trdp_spy c:\wireshark-%WS_VERSION%\plugins\trdp_spy\
xcopy /Y /S %LIBXML_DIR% c:\wireshark-%WS_VERSION%\plugins\trdp_spy\%LIBXML%
xcopy /Y /S %ICONV_DIR% c:\wireshark-%WS_VERSION%\plugins\trdp_spy\%ICONV%

c:
cd "C:\Program Files\Microsoft Visual Studio 10.0\VC\"
call "bin\vcvars32.bat"

echo ====================== build the expected wireshark version ======================
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

echo ====================== build the plugin ======================
cd plugins\trdp_spy
nmake -f Makefile.nmake distclean
nmake -f Makefile.nmake all

IF     ERRORLEVEL 1 goto :quit

copy trdp_spy.dll C:\wireshark-%WS_VERSION%\wireshark-gtk2\plugins\%WS_VERSION%
copy C:\wireshark-%WS_VERSION%\wireshark-gtk2\libxml2-2.dll C:\wireshark-%WS_VERSION%\wireshark-gtk2\libxml2.dll

echo === Copy the new version into the output location of the SVN ===
copy trdp_spy.dll %SVN_OUTPUT%

goto :end

:quit
echo ERROR..!!build fail

:end
rem go back to the directory we came from
%WORKINGDISK%
cd %WORKINGDIR%