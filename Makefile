#//
#// $Id$
#//
#// DESCRIPTION    trdp Makefile
#//
#// AUTHOR         Bernd Loehr, NewTec GmbH
#//
#// This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
#// If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#// Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013-2016. All rights reserved.
#//
#// BL 2016-02-11: Ticket #88 Cleanup makefiles, remove dependencies on external libraries


#// Support for POSIX and VXWORKS, set buildsettings and config first!
 .EXPORT_ALL_VARIABLES:
# Check if configuration is present
ifeq (config/config.mk,$(wildcard config/config.mk)) 
# load target specific configuration
include config/config.mk
endif

include rules.mk

MD = mkdir -p
	
CFLAGS += -D$(TARGET_OS)

# Set paths
INCPATH += -I src/api
VOS_PATH = -I src/vos/$(TARGET_VOS)
VOS_INCPATH = -I src/vos/api -I src/common

vpath %.c src/common src/vos/common test/udpmdcom src/vos/$(TARGET_VOS) test example test/diverse test/xml
vpath %.h src/api src/vos/api src/common src/vos/common

INCLUDES = $(INCPATH) $(VOS_INCPATH) $(VOS_PATH)

#set up your specific paths for the lint *.lnt files etc
#by setting up an environment variable LINT_RULE_DIR like below
#LINT_RULE_DIR = /home/mkoch/svn_trdp/lint/pclint/9.00i/lnt/

#also you may like to point to a spcific path for the lint
#binary - have the environment variable LINT_BINPATH available
#within your build shell
FLINT = $(LINT_BINPATH)flint

# Set Objects
VOS_OBJS = vos_utils.o \
	   vos_sock.o \
	   vos_mem.o \
	   vos_thread.o \
	   vos_shared_mem.o

TRDP_OBJS = trdp_pdcom.o \
	    trdp_utils.o \
	    trdp_if.o \
	    trdp_stats.o \
	    $(VOS_OBJS)

# Optional objects for full blown TRDP usage
TRDP_OPT_OBJS = trdp_xml.o \
		tau_xml.o \
		tau_marshall.o \
		tau_dnr.o \
		tau_tti.o \
		tau_ctrl.o


# Set LINT Objects
LINT_OBJECTS = trdp_stats.lob\
           vos_utils.lob \
	   vos_sock.lob \
	   vos_mem.lob \
	   vos_thread.lob \
	   vos_shared_mem.lob \
	   trdp_pdcom.lob \
	   trdp_mdcom.lob \
	   trdp_utils.lob \
	   trdp_if.lob \
	   trdp_stats.lob     

# Set LDFLAGS
LDFLAGS += -L $(OUTDIR)

# Enable / disable MD Support
# by default MD_SUPPORT is always enabled (in current state)
ifeq ($(MD_SUPPORT),0)
CFLAGS += -DMD_SUPPORT=0
else
TRDP_OBJS += trdp_mdcom.o
CFLAGS += -DMD_SUPPORT=1
endif

ifeq ($(DEBUG), TRUE)
	OUTDIR = bld/output/$(ARCH)-dbg
else
	OUTDIR = bld/output/$(ARCH)-rel
endif

# Set LINT result outdir now after OUTDIR is known
LINT_OUTDIR  = $(OUTDIR)/lint
  
# Enable / disable Debug
ifeq ($(DEBUG),TRUE)
CFLAGS += -g3 -O -DDEBUG
LDFLAGS += -g3
# Display the strip command and do not execute it
STRIP = @echo "do NOT strip: "
else
CFLAGS += -Os  -DNO_DEBUG
endif

TARGETS = outdir libtrdp

ifneq ($(TARGET_OS),VXWORKS)
TARGETS += example test pdtest mdtest xml
else
TARGETS += vtests
endif

all:	$(TARGETS)

outdir:
	@$(MD) $(OUTDIR)

libtrdp:	outdir $(OUTDIR)/libtrdp.a

example:	outdir $(OUTDIR)/receiveSelect $(OUTDIR)/cmdlineSelect $(OUTDIR)/receivePolling $(OUTDIR)/sendHello $(OUTDIR)/receiveHello

test:		outdir $(OUTDIR)/getStats $(OUTDIR)/vostest $(OUTDIR)/test_mdSingle $(OUTDIR)/inaugTest

pdtest:		outdir $(OUTDIR)/trdp-pd-test $(OUTDIR)/pd_md_responder $(OUTDIR)/testSub

mdtest:		outdir $(OUTDIR)/trdp-md-test $(OUTDIR)/trdp-md-reptestcaller $(OUTDIR)/trdp-md-reptestreplier

vtests:		outdir $(OUTDIR)/vtest

xml:		outdir $(OUTDIR)/trdp-xmlprint-test $(OUTDIR)/trdp-xmlpd-test



%_config:
	cp -f config/$@ config/config.mk

$(OUTDIR)/%.o: %.c %.h trdp_if_light.h trdp_types.h vos_types.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTDIR)/libtrdp.a:		$(addprefix $(OUTDIR)/,$(notdir $(TRDP_OBJS)))
			@echo ' ### Building the lib $(@F)'
			$(RM) $@
			$(AR) cq $@ $^


###############################################################################
#
# rules for the demos
#
###############################################################################
				  
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

$(OUTDIR)/receiveHello: receiveHello.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building application $(@F)'
			$(CC) example/receiveHello.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
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

###############################################################################
#
# rule for the example
#
###############################################################################
$(OUTDIR)/mdManager: mdManager.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building example MD application $(@F)'
			$(CC) example/mdManager.c \
			    -ltrdp \
			    -luuid \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

###############################################################################
#
# rule for the various test binaries
#
###############################################################################
$(OUTDIR)/trdp-xmlprint-test:  trdp-xmlprint-test.c  $(OUTDIR)/libtrdp.a $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) $^ \
			$(CFLAGS) $(INCLUDES) -o $@ \
			-ltrdp -lz \
			$(LDFLAGS)
			$(STRIP) $@

$(OUTDIR)/trdp-xmlpd-test:  trdp-xmlpd-test.c  $(OUTDIR)/libtrdp.a $(OUTDIR)/libtrdp.a $(addprefix $(OUTDIR)/,$(notdir $(TRDP_OPT_OBJS)))
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) $^  \
			$(CFLAGS) $(INCLUDES) -o $@\
			-ltrdp -lz \
			$(LDFLAGS)
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

$(OUTDIR)/inaugTest:   diverse/inaugTest.c  $(OUTDIR)/libtrdp.a
			@echo ' ### Building republish test $(@F)'
			$(CC) test/diverse/inaugTest.c \
			    -ltrdp \
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

$(OUTDIR)/trdp-md-reptestcaller: $(OUTDIR)/libtrdp.a
			@echo ' ### Building MD test Caller application $(@F)'
			$(CC) test/mdpatterns/rep-testcaller.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/trdp-md-reptestreplier: $(OUTDIR)/libtrdp.a
			@echo ' ### Building MD test Replier application $(@F)'
			$(CC) test/mdpatterns/rep-testreplier.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/vtest: $(OUTDIR)/libtrdp.a
			@echo ' ### Building vtest application $(@F)'
			$(CC) test/diverse/vtest.c \
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
			
###############################################################################
#
# wipe out everything section - except the previous target configuration
#
###############################################################################
clean:
	rm -f -r bld/*
	rm -f -r doc/latex/*   

unconfig:
	-$(RM) config/config.mk
	
distclean:	clean unconfig	 		

###############################################################################
#                                                                      	
# Common lint for the whole system
#
############################################################################### 
lint:   loutdir $(LINT_OUTDIR)/final.lint

loutdir:
	@$(MD) $(LINT_OUTDIR)

LINTFLAGS = +v -i$(LINT_RULE_DIR) $(LINT_RULE_DIR)co-gcc.lnt -i ./src/api -i ./src/vos/api -i ./src/common -D$(TARGET_OS) $(LINT_SYSINCLUDE_DIRECTIVES)\
	-DMD_SUPPORT=1 -w3 -e655 -summary -u

# VxWorks will be the single non POSIX OS right now, MS Win uses proprietary build
# framework, so this condition will most likely fit also BSD/Unix targets     
ifneq ($(TARGET_OS),VXWORKS)
LINTFLAGS += -DPOSIX
endif
	
	
$(LINT_OUTDIR)/final.lint: $(addprefix $(LINT_OUTDIR)/,$(notdir $(LINT_OBJECTS)))
	@$(ECHO) '### Lint Final'
	@$(ECHO) '### Final Lint Stage - Verifying inter module / system wide stuff' > $@
	$(FLINT) $(LINTFLAGS) $(SILENCE_LINT) -zero  $^ 1>>$@ 2>>$@  

# Partial lint for single C files
$(LINT_OUTDIR)/%.lob : %.c
	@$(ECHO) ' --- Lint $(^F)'
	@$(ECHO) '### Lint file: $(^F)'  > $(LINT_OUTDIR)/$(^F).lint
	$(FLINT) $(LINTFLAGS) -u -oo\($@\)  -zero $< 1>>$(LINT_OUTDIR)/$(^F).lint 2>>$(LINT_OUTDIR)/$(^F).lint	

###############################################################################
#
# create doxygen documentation
#
###############################################################################
doc:		doc/latex/refman.pdf

doc/latex/refman.pdf: Doxyfile trdp_if_light.h trdp_types.h
			@echo ' ### Making the Reference Manual PDF'
			$(DOXYPATH)doxygen Doxyfile
			make -C doc/latex
			cp doc/latex/refman.pdf "doc/TCN-TRDP2-D-BOM-033-xx - TRDP Reference Manual.pdf"
                                                 

###############################################################################
#
# help section for available build options
#
###############################################################################
help:
	@echo " " >&2
	@echo "BUILD TARGETS FOR TRDP" >&2
	@echo "Load one of the configurations below with 'make <configuration>' first:" >&2 
	@echo "  " >&2
	@echo "  * LINUX_config					- Native build for Linux (uses host gcc regardless of 32/64 bit)" >&2
	@echo "  * LINUX_X86_config             - Native build for Linux (Little Endian, uses host gcc regardless of 32/64 bit)" >&2
	@echo "  * LINUX_PPC_config             - Building for Linux on PowerPC using eglibc compiler (603 core)" >&2
	@echo "  * LINUX_ARM_OTN_config         - Building for Linux on Xscale" >&2
	@echo "  * OSX_X86_config               - Native (X86) build for OS X" >&2
	@echo "  * QNX_X86_config               - Native (X86) build for QNX" >&2	
	@echo "  * VXWORKS_KMODE_PPC_config     - Building for VXWORKS kernel mode for PowerPC (experimental)" >&2
	@echo " " >&2
	@echo "Custom adaptation hints:" >&2
	@echo " " >&2	
	@echo "a) create a buildsettings_%target file containing the paths for compilers etc. prepared to be uesd as source file for your build shell" >&2
	@echo "b) adapt the most fitting *_config file to your needs and add it for comprehensibilty to the above list." >&2
	@echo " " >&2	
	@echo "Then call 'make' or 'make all' to build everything. Contents of target all marked below" >&2
	@echo "in the 'Other builds:' list with #" >&2
	@echo "To build debug binaries, append 'DEBUG=TRUE' to the make command " >&2
	@echo "To include message data support, append 'MD_SUPPORT=1' to the make command " >&2
	@echo " " >&2
	@echo "Other builds:" >&2
	@echo "  * make demo      # build the sample applications" >&2
	@echo "  * make test      # build the test server application" >&2
	@echo "  * make pdtest    # build the PDCom test applications" >&2
	@echo "  * make mdtest    # build the UDPMDcom test application" >&2
	@echo "  * make example   # build the example for MD communication, but needs libuuid!" >&2
	@echo "  * make libtrdp   # build the static library, only" >&2
	@echo "  * make xml       # build the xml test applications" >&2
	@echo " " >&2
	@echo "Static analysis (currently in prototype state) " >&2
	@echo "  * make lint      - build LINT analysis files using the LINT binary under $FLINT" >&2	
	@echo " " >&2	
	@echo "Build tree clean up helpers" >&2	
	@echo "  * make clean     - remove all binaries and objects of the current target" >&2
	@echo "  * make unconfig  - remove the configuration file" >&2
	@echo "  * make distclean - make clean unconfig" >&2
	@echo " " >&2	
	@echo "Documentation generator" >&2	
	@echo "  * make doc       - build the documentation (refman.pdf)" >&2
	@echo "                   - (needs doxygen installed in executable path)" >&2
	@echo " " >&2


