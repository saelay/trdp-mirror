                                                               
                  _____ _____ ____  _____    _____ _____ __ __ 
                 |_   _| __  |    \|  _  |  |   __|  _  |  |  |
                   | | |    -|  |  |   __|  |__   |   __|_   _|
                   |_| |__|__|____/|__|     |_____|__|    |_|  
                                                               

															   
This Plugin can be used to display packages containing TRDP (Train Realtime Data Protocol).

The Plugin was developed for wireshark version 1.8.*.

As the build environment in 1.8.3 was moved from Makefiles to CMake.
There is another Makefile for 1.8.3 and following versions.


B U I L D I N G
===============

For Windows:
 Wireshark-Version 1.8.3
	build-1_8_3.bat
 Wireshark-Version 1.8.0
	build.bat

The both batch files must probalby be adapted to the installed Visual-Studio Version.
The in the file clean.bat, also the installed visual studio version must be set.


For Linux:
	Makefile

In not already done, libxml2 must be installed as development version on the host.

C L E A N
=========

For Windows:
	clean.bat
	
For Linux:
	make clean