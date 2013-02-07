#//
#// $Id$
#//
#// DESCRIPTION    trdp Makefile
#//
#// AUTHOR         Bernd Loehr, NewTec GmbH
#//
#// All rights reserved. Reproduction, modification, use or disclosure
#// to third parties without express authority is forbidden.
#// Copyright Bombardier Transportation GmbH, Germany, 2012.
#//

# Preliminary: Currently, posix support only

TARGET_VOS = posix
TARGET_FLAG = POSIX

# --------------------------------------------------------------------

VOS_PATH = src/vos/$(TARGET_VOS)

vpath %.c src/common src/example src/vos/common test/udpmdcom $(VOS_PATH) test
vpath %.h src/api src/vos/api src/common src/vos/common

INCPATH = src/api
VOS_INCPATH = src/vos/api -I src/common
BUILD_PATH = bld/$(TARGET_VOS)
DOXYPATH = /usr/local/bin/

ifdef TARGET
DIR_PATH=$(TARGET)/
CROSS=$(TARGET)-

CC=$(CROSS)gcc
AR=$(CROSS)ar
LD=$(CROSS)ld
STRIP=$(CROSS)strip
else
CC = $(GNUPATH)gcc
AR = $(GNUPATH)ar
LD = $(GNUPATH)ld
STRIP = $(GNUPATH)strip
endif

ECHO = echo
RM = rm -f
MD = mkdir -p
CP = cp


ifeq ($(MD_SUPPORT),1)
CFLAGS += -D__USE_BSD -D_DARWIN_C_SOURCE -D_XOPEN_SOURCE=500 -pthread -D$(TARGET_FLAG)  -fPIC -Wall -DMD_SUPPORT=1
else
CFLAGS += -D__USE_BSD -D_DARWIN_C_SOURCE -D_XOPEN_SOURCE=500 -pthread -D$(TARGET_FLAG)  -fPIC -Wall -DMD_SUPPORT=0
endif

SUBDIRS	= src
INCLUDES = -I $(INCPATH) -I $(VOS_INCPATH) -I $(VOS_PATH)
OUTDIR = $(BUILD_PATH)

LDFLAGS = -L $(OUTDIR)
# files, all tests need to run 
SRC_TEST = test/test_general.c

ifeq ($(DEBUG),1)
CFLAGS += -g -O -DDEBUG
LDFLAGS += -g
# Display the strip command and do not execut it
STRIP = $(ECHO) "do NOT strip: " 
else
CFLAGS += -Os  -DNO_DEBUG
endif

VOS_OBJS = vos_utils.o vos_sock.o vos_mem.o vos_thread.o
TRDP_OBJS = trdp_pdcom.o trdp_utils.o trdp_if.o trdp_stats.o $(VOS_OBJS)

ifeq ($(MD_SUPPORT),1)
TRDP_OBJS += trdp_mdcom.o
endif

all:		outdir libtrdp demo

libtrdp:	outdir $(OUTDIR)/libtrdp.a
demo:		outdir $(OUTDIR)/receiveSelect $(OUTDIR)/cmdlineSelect $(OUTDIR)/receivePolling $(OUTDIR)/sendHello $(OUTDIR)/mdManagerTCP $(OUTDIR)/mdManagerTCP_Siemens
example:	outdir $(OUTDIR)/mdManager
test:		outdir $(OUTDIR)/getstats

pdtest:		outdir $(OUTDIR)/trdp-pd-test
mdtest:		outdir $(OUTDIR)/mdTest0001		$(OUTDIR)/mdTest0002

doc:		doc/latex/refman.pdf

$(OUTDIR)/trdp_if.o:	trdp_if.c
			$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OUTDIR)/%.o: %.c %.h trdp_if_light.h trdp_types.h vos_types.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


$(OUTDIR)/libtrdp.a:	$(addprefix $(OUTDIR)/,$(notdir $(TRDP_OBJS)))
			@$(ECHO) ' ### Building the lib $(@F)'
			$(RM) $@
			$(AR) cq $@ $^


$(OUTDIR)/receiveSelect:  echoSelect.c  $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) $(SUBDIRS)/example/echoSelect.c \
				$(CFLAGS) $(INCLUDES) -o $@\
				-ltrdp \
			    $(LDFLAGS) \
			    
			$(STRIP) $@

			
$(OUTDIR)/cmdlineSelect:  echoSelect.c  $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) $(SUBDIRS)/example/echoSelectcmdline.c \
				$(CFLAGS) $(INCLUDES) -o $@\
				-ltrdp \
			    $(LDFLAGS) \
			    
			$(STRIP) $@
			
$(OUTDIR)/receivePolling:  echoPolling.c  $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) $(SUBDIRS)/example/echoPolling.c \
				$(CFLAGS) $(INCLUDES) -o $@\
				-ltrdp \
			    $(LDFLAGS) \
			    
			$(STRIP) $@


$(OUTDIR)/sendHello:   sendHello.c  $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building application $(@F)'
			$(CC) $(SUBDIRS)/example/sendHello.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/getstats:   getStats.c  $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building statistics commandline tool $(@F)'
			$(CC) test/getStats.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@
			
$(OUTDIR)/mdManager: mdManager.c  $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building example MD application $(@F)'
			$(CC) $(SUBDIRS)/example/mdManager.c \
			    -ltrdp \
			    -luuid -lrt \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/mdTest0001: mdTest0001.c  $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building UDPMDCom test application $(@F)'
			$(CC) test/udpmdcom/mdTest0001.c \
			    -ltrdp \
			    -luuid -lrt \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/mdTest0002: mdTest0002.c  $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building UDPMDCom test application $(@F)'
			$(CC) test/udpmdcom/mdTest0002.c \
			    -ltrdp \
			    -luuid -lrt \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@
			
$(OUTDIR)/test_mdSingle: $(OUTDIR)/libtrdp.a 
			$(CC) test/test_mdSingle.c \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@


$(OUTDIR)/mdManagerTCP_Siemens: mdManagerTCP_Siemens.c  $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building TCPMDCom Siemens test application $(@F)'
			$(CC) $(SUBDIRS)/example/mdManagerTCP_Siemens.c \
			    -ltrdp \
			    -luuid -lrt \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@


$(OUTDIR)/mdManagerTCP: mdManagerTCP.c  $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building TCPMDCom test application $(@F)'
			$(CC) $(SUBDIRS)/example/mdManagerTCP.c \
			    -ltrdp \
			    -luuid -lrt \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@

$(OUTDIR)/trdp-pd-test: $(OUTDIR)/libtrdp.a 
			@$(ECHO) ' ### Building PD test application $(@F)'
			$(CC) test/pdpatterns/trdp-pd-test.cpp \
			    -ltrdp \
			    $(LDFLAGS) $(CFLAGS) $(INCLUDES) \
			    -o $@
			$(STRIP) $@
			
outdir:
		$(MD) $(OUTDIR)


doc/latex/refman.pdf: Doxyfile trdp_if_light.h trdp_types.h
			@$(ECHO) ' ### Making the PDF document'
			$(DOXYPATH)/doxygen Doxyfile
			make -C doc/latex
			$(CP) doc/latex/refman.pdf doc



help:
	@echo " " >&2
	@echo "BUILD TARGETS FOR TRDP" >&2
	@echo "Edit the paths in the top part of this Makefile to suit your environment." >&2
	@echo "Then call 'make' or 'make all' to build everything." >&2
	@echo "To build debug binaries, append 'DEBUG=TRUE' to the make command " >&2
	@echo "To include message data support, append 'MD_SUPPORT=1' to the make command " >&2
	@echo " " >&2
	@echo "Other builds:" >&2
	@echo "  * make demo      - build the sample applications" >&2
	@echo "  * make test      - build the test server application" >&2
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
	$(RM) -r bld/*
	$(RM) -r doc/latex/*


#########################################################################
