@echo off
rem environment:
SET WS_VERSION=1.8.3
SET WS_WORKDIR=c:\wireshark-%WS_VERSION%
SET WS_WORKDIR_DriveLetter=c:
SET MS_VisualStudio_DIR=C:\Program Files\Microsoft Visual Studio 10.0\VC\
set MS_VisualStudio_DriveLetter=c:

rem proxy settings to leave the company's network:
set http_proxy=webproxy.apac.bombardier.com:8080

rem internal variables:
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
SET WS_TOOLS_DIR=%WS_WORKDIR%\tools
set WS_WORKING_TRDP_SPY=%WS_WORKDIR%\plugins\trdp_spy
set WS_WORKING_OUTPUT_DIR=%WS_WORKDIR%\wireshark-gtk2

echo ====================== TRDP-SPY setup ======================
rem copy the plugin to the place where it is needed to be when compiling
rem prepared the target folder
mkdir %WS_WORKING_TRDP_SPY%\
mkdir %WS_WORKING_TRDP_SPY%\%LIBXML%
mkdir %WS_WORKING_TRDP_SPY%\%ICONV%
rem copy this directory with the source code into the folder with the wireshark source code
cd ..
copy trdp_spy %WS_WORKING_TRDP_SPY%
xcopy /Y /S %LIBXML_DIR% %WS_WORKING_TRDP_SPY%\%LIBXML%
xcopy /Y /S %ICONV_DIR%  %WS_WORKING_TRDP_SPY%\%ICONV%

rem load the Visual studio environment
call %MS_VisualStudio_DriveLetter%
cd %MS_VisualStudio_DIR%
call "bin\vcvars32.bat"

echo ====================== build the expected wireshark version ======================
call %WS_WORKDIR_DriveLetter%
cd %WS_WORKDIR%

rem mkdir %WS_VERSION%
rem cd %WS_VERSION%
rem svn co http://anonsvn.wireshark.org/wireshark/releases/wireshark-1.8.0/

rem ------------------------  modify the fileending for windows
cd %WS_TOOLS_DIR%
dos2unix win32-setup.sh
dos2unix textify.sh
cd %WS_WORKDIR%

rem nmake -f Makefile.nmake verify_tools
rem nmake -f Makefile.nmake setup
rem nmake -f Makefile.nmake distclean
nmake -f Makefile.nmake all
rem nmake -f Makefile.nmake wireshark.bsc

echo ====================== build the plugin ======================
cd %WS_WORKING_TRDP_SPY%
nmake -f Makefile.nmake distclean
nmake -f Makefile.nmake all

IF     ERRORLEVEL 1 goto :quit

copy trdp_spy.dll %WS_WORKING_OUTPUT_DIR%\plugins\%WS_VERSION%
copy %WS_WORKING_OUTPUT_DIR%\libxml2-2.dll %WS_WORKING_OUTPUT_DIR%\libxml2.dll

echo === Copy the new version into the output location of the SVN ===
copy trdp_spy.dll %SVN_OUTPUT%

goto :end

:quit
echo ERROR..!!build fail

:end
rem go back to the directory we came from
%WORKINGDISK%
cd %WORKINGDIR%