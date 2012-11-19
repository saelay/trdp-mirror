UNAME := $(shell uname)

VOS_POSIX=src/vos/posix
VOS_CMM=src/vos/common
VOS_INC=src/vos/api
COM_CMM=src/common
COM_INC=src/common
API_INC=src/api

RM=rm -f

BLDDIR=bld/posix

PROJ_MDM=$(BLDDIR)/mdManager
PROJ_MDM1=$(BLDDIR)/mdManager1
PROJ_MDM2=$(BLDDIR)/mdManager2
PROJ_LOG=$(BLDDIR)/miscLog

TRDPLIB=$(BLDDIR)/libtrdp.a

OSLIB= -ltrdp
ifeq ($(UNAME),Linux)
OSLIB += -lpthread
OSLIB += -luuid
OSLIB += -lrt
endif
ifeq ($(UNAME),QNX)
OSLIB += -lsocket
endif

OBJLIB=\
	$(COM_CMM)/trdp_stats.o \
	$(COM_CMM)/trdp_mdcom.o \
	$(COM_CMM)/trdp_pdcom.o \
	$(COM_CMM)/trdp_utils.o \
	$(COM_CMM)/trdp_if.o \
		\
	$(VOS_CMM)/vos_utils.o \
		\
	$(VOS_POSIX)/vos_sock.o \
	$(VOS_POSIX)/vos_thread.o \
	$(VOS_POSIX)/vos_mem.o
		

OBJPRJ_MDM= src/example/mdManager.o
OBJPRJ_MDM1= src/example/mdManager1.o
OBJPRJ_MDM2= src/example/mdManager2.o
OBJPRJ_LOG= src/example/miscLog.o

CFLAGS=-Wall -D_GNU_SOURCE -DPOSIX -I$(VOS_INC) -I$(COM_INC) -I$(API_INC) -DMD_SUPPORT=1

all: $(TRDPLIB) $(PROJ_MDM) $(PROJ_MDM1) $(PROJ_MDM2)
#Removed project, as the source is not available $(PROJ_LOG) 
$(PROJ_MDM) : $(TRDPLIB) $(OBJPRJ_MDM) 
	$(CC) -o $(PROJ_MDM) $(OBJPRJ_MDM) -L $(BLDDIR) $(OSLIB)

$(PROJ_MDM1) : $(TRDPLIB) $(OBJPRJ_MDM1) 
	$(CC) -o $(PROJ_MDM1) $(OBJPRJ_MDM1) -L $(BLDDIR) $(OSLIB)

$(PROJ_MDM2) : $(TRDPLIB) $(OBJPRJ_MDM2) 
	$(CC) -o $(PROJ_MDM2) $(OBJPRJ_MDM2) -L $(BLDDIR) $(OSLIB)

#$(PROJ_LOG) : $(TRDPLIB) $(OBJPRJ_LOG) 
#	$(CC) -o $(PROJ_LOG) $(OBJPRJ_LOG) -L $(BLDDIR) $(OSLIB)

$(TRDPLIB) : $(OBJLIB)
	$(AR) cr $(TRDPLIB) $(OBJLIB)

clean:
	$(RM) $(OBJLIB) $(OBJPRJ_MDM) $(TRDPLIB) $(PROJ_MDM) $(PROJ_LOG) $(OBJPRJ_LOG) $(PROJ_MDM1) $(OBJPRJ_MDM1) $(PROJ_MDM2) $(OBJPRJ_MDM2)
