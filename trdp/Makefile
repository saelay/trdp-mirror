#//
#// $Id$
#//
#// DESCRIPTION    trdp Makefile
#//
#// AUTHOR         Bernd Loehr, NewTec GmbH
#//
#// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#// Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
#//

#// Support for POSIX and VXWORKS, set buildsettings and config first!

# Check if configuration is present
ifeq (config/config.mk,$(wildcard config/config.mk)) 
# load target specific configuration
include config/config.mk
endif

# Set paths
INCPATH += -I src/api
VOS_PATH = -I src/vos/$(TARGET_VOS)
VOS_INCPATH = -I src/vos/api -I src/common
BUILD_PATH = bld/$(TARGET_VOS)
vpath %.c src/common  src/vos/common test/udpmdcom $(VOS_PATH) test example test/diverse
vpath %.h src/api src/vos/api src/common src/vos/common
SUBDIRS	= src
INCLUDES = $(INCPATH) $(VOS_INCPATH) $(VOS_PATH)
OUTDIR = $(BUILD_PATH)
# path to doxygen binary
DOXYPATH = /usr/local/bin/

# Set Objects
VOS_OBJS = vos_utils.o vos_sock.o vos_mem.o vos_thread.o vos_shared_mem.o
TRDP_OBJS = trdp_pdcom.o trdp_utils.o trdp_if.o trdp_stats.o $(VOS_OBJS)

# Set LDFLAGS
LDFLAGS += -L $(OUTDIR)

# Enable / disable MD Support
ifeq ($(MD_SUPPORT),0)
CFLAGS += -DMD_SUPPORT=0
else
TRDP_OBJS += trdp_mdcom.o
CFLAGS += -DMD_SUPPORT=1
endif

# adapt for operating system
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
# flags needed for Linux
CFLAGS += -D_GNU_SOURCE
endif
ifeq ($(UNAME), Darwin)
# flags needed for OSX
CFLAGS += -D__USE_BSD -D_DARWIN_C_SOURCE
endif

# Enable / disable Debug
ifeq ($(DEBUG),1)
CFLAGS += -g -O -DDEBUG
LDFLAGS += -g
# Display the strip command and do not execute it
STRIP = @echo "do NOT strip: "
else
CFLAGS += -Os  -DNO_DEBUG
endif

AS = $(TCPREFIX)as$(TCPOSTFIX)
LD = $(TCPREFIX)ld$(TCPOSTFIX)
CC = $(TCPREFIX)gcc$(TCPOSTFIX)
CPP	= $(CC) -E
AR = $(TCPREFIX)ar$(TCPOSTFIX)
NM = $(TCPREFIX)nm$(TCPOSTFIX)
STRIP = $(TCPREFIX)strip$(TCPOSTFIX)

# Special setting for VXWORKS
ifeq ($(TARGET_FLAG),VXWORKS)
CC = $(TCPREFIX)cc$(TCPOSTFIX)
endif

all:	outdir libtrdp demo example test pdtest mdtest

libtrdp:	outdir $(OUTDIR)/libtrdp.a

demo:		outdir $(OUTDIR)/receiveSelect $(OUTDIR)/cmdlineSelect $(OUTDIR)/receivePolling $(OUTDIR)/sendHello $(OUTDIR)/mdManagerTCP $(OUTDIR)/mdManagerTCP_Siemens
example:	outdir $(OUTDIR)/mdManager
test:		outdir $(OUTDIR)/getStats $(OUTDIR)/vostest $(OUTDIR)/test_mdSingle

pdtest:		outdir $(OUTDIR)/trdp-pd-test $(OUTDIR)/pd_md_responder $(OUTDIR)/testSub
mdtest:		outdir $(OUTDIR)/trdp-md-test $(OUTDIR)/mdTest4

doc:		doc/latex/refman.pdf

%_config:
	cp -f config/$@ config/config.mk
	
$(OUTDIR)/trdp_if.o:	trdp_if.c
			$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTDIR)/%.o: %.c %.h trdp_if_light.h trdp_types.h vos_types.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTDIR)/libtrdp.a:	$(addprefix $(OUTDIR)/,$(notdir $(TRDP_OBJS)))
			@echo ' ### Building the lib $(@F)'
			$(RM) $@
			$(AR) cq $@ $^

$(OUTDIR)/receiveSelect:  echoSelect.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building application $(@F)'
			$(CC) example/echoSelect.c \
				$(CFLAGS) $(INCLUDES) -o $@\
				-ltrdp \
			    $(LDFLAGS)
			$(STRIP) $@

$(OUTDIR)/cmdlineSelect:  echoSelectcmdline.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building application $(@F)'
			$(CC) example/echoSelectcmdline.c \
				$(CFLAGS) $(INCLUDES) -o $@\
				-ltrdp \
			    $(LDFLAGS) 
			$(STRIP) $@

$(OUTDIR)/receivePolling:  echoPolling.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building application $(@F)'
			$(CC) example/echoPolling.c \
				$(CFLAGS) $(INCLUDES) -o $@\
				-ltrdp \
			    $(LDFLAGS)
			$(STRIP) $@

$(OUTDIR)/sendHello:   sendHello.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building application $(@F)'
			$(CC) example/sendHello.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/getStats:   diverse/getStats.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building statistics commandline tool $(@F)'
			$(CC) test/diverse/getStats.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/mdManager: mdManager.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building example MD application $(@F)'
			$(CC) example/mdManager.c \
			    -ltrdp \
			    -luuid \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/mdTest4: mdTest4.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building UDPMDCom test application $(@F)'
			$(CC) test/udpmdcom/mdTest4.c \
			    -ltrdp \
			    -luuid \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/test_mdSingle: test_mdSingle.c $(OUTDIR)/libtrdp.a
			@echo ' ### Building MD single test application $(@F)'
			$(CC) test/diverse/test_mdSingle.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/mdManagerTCP_Siemens: mdManagerTCP_Siemens.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building TCPMDCom Siemens test application $(@F)'
			$(CC) example/mdManagerTCP_Siemens.c \
			    -ltrdp \
			    -luuid \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/mdManagerTCP: mdManagerTCP.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building TCPMDCom test application $(@F)'
			$(CC) example/mdManagerTCP.c \
			    -ltrdp \
			    -luuid \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/trdp-pd-test: $(OUTDIR)/libtrdp.a
			@echo ' ### Building PD test application $(@F)'
			$(CC) test/pdpatterns/trdp-pd-test.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/trdp-md-test: $(OUTDIR)/libtrdp.a
			@echo ' ### Building MD test application $(@F)'
			$(CC) test/mdpatterns/trdp-md-test.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/vostest: $(OUTDIR)/libtrdp.a
			@echo ' ### Building VOS test application $(@F)'
			$(CC) test/diverse/LibraryTests.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/pd_md_responder: $(OUTDIR)/libtrdp.a pd_md_responder.c
			@echo ' ### Building PD test application $(@F)'
			$(CC) test/diverse/pd_md_responder.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/testSub: $(OUTDIR)/libtrdp.a subTest.c
			@echo ' ### Building subscribe PD test application $(@F)'
			$(CC) test/diverse/subTest.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

outdir:
	mkdir -p $(OUTDIR)

doc/latex/refman.pdf: Doxyfile trdp_if_light.h trdp_types.h
			@echo ' ### Making the Reference Manual PDF'
			$(DOXYPATH)/doxygen Doxyfile
			make -C doc/latex
			cp doc/latex/refman.pdf "doc/TCN-TRDP2-D-BOM-033-xx - TRDP Reference Manual.pdf"

help:
	@echo " " >&2
	@echo "BUILD TARGETS FOR TRDP" >&2
	@echo "Edit the paths in the buildsettings_%target file to prepare your build shell and apply changes to your environment variables." >&2
	@echo "Then call 'make %config' to copy target specific config settings to 'config/config.mk', e.g. 'make POSIX_X86_config' " >&2
	@echo "Then call 'make' or 'make all' to build everything." >&2
	@echo "To build debug binaries, append 'DEBUG=TRUE' to the make command " >&2
	@echo "To include message data support, append 'MD_SUPPORT=1' to the make command " >&2
	@echo " " >&2
	@echo "Other builds:" >&2
	@echo "  * make demo      - build the sample applications" >&2
	@echo "  * make test      - build the test server application" >&2
	@echo "  * make pdtest    - build the PDCom test applications" >&2
	@echo "  * make mdtest    - build the UDPMDcom test application" >&2
	@echo "  * make example   - build the example for MD communication, but needs libuuid!" >&2
	@echo "  * make clean     - remove all binaries and objects of the current target" >&2
	@echo "  * make libtrdp	  - build the static library, only" >&2
	@echo " " >&2
	@echo "  * make doc	      - build the documentation (refman.pdf)" >&2
	@echo "                   - (needs doxygen installed in executable path)" >&2
	@echo " " >&2

#########################################################################

clean:
	rm -f -r bld/*
	rm -f -r doc/latex/*

#########################################################################
