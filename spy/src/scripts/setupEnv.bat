@echo off
set CYGWIN=nodosfilewarning
set WIRESHARK_BASE_DIR=D:\src\Wireshark\wireshark-custom
set WIRESHARK_TARGET_PLATFORM=win64
set QT5_BASE_DIR=C:\Qt\Qt5.6.2\5.6\msvc2013_64

set WIRESHARK_VERSION_EXTRA=TRDP
rem set POWERSHELL=C:\Windows\winsxs\amd64_microsoft-windows-powershell-exe_\
set PATH=%POWERSHELL%;%PATH%;C:\Program Files\CMake\bin
set PLATFORM=win64

rem Add the Visual Studio Build script
set PATH=%PATH%;C:\Program Files (x86)\MSBuild\12.0\Bin

rem If your Cygwin installation path is not automatically detected by CMake:
rem set WIRESHARK_CYGWIN_INSTALL_PATH=c:\cygwin