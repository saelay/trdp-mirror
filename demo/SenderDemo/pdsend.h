/*
 *  pdsend.h
 *  SenderDemo
 *
 *  Created by Bernd Löhr on 22.11.11.
 *  Copyright 2011 LB Electronics. All rights reserved.
 *
 */

#include "trdp_if_light.h"
#include "vos_thread.h"

#define PD_COMID0			2000
#define PD_COMID0_CYCLE		1000000
#define PD_COMID0_TIMEOUT	3200000

#define PD_COMID1			2001
#define PD_COMID1_CYCLE		100000
#define PD_COMID1_TIMEOUT	1200000

#define PD_COMID2			2002
#define PD_COMID2_CYCLE		100000
#define PD_COMID2_TIMEOUT	1200000

#define PD_COMID3			2003
#define PD_COMID3_CYCLE		100000
#define PD_COMID3_TIMEOUT	1200000


typedef struct pd_receive_packet {
    TRDP_SUB_T  subHandle;
    uint32_t	comID;
	uint32_t	timeout;
    char        srcIP[16];
    uint32_t    counter;
    uint8_t     message[64];
    int         changed;
    int         invalid;
} pd_receive_packet_t;


int pd_init(
	const char*	pDestAddress,
	uint32_t	comID,
	uint32_t	interval);

void pd_deinit();

void pd_stop(BOOL redundant);

void pd_updatePublisher(BOOL stop);
void pd_updateSubscriber(int index);
void pd_updateData(
	uint8_t	*pData,
	size_t	dataSize);
void pd_sub(pd_receive_packet_t*    recPacket);
pd_receive_packet_t* pd_get(int index);

void setIP(const char* ipAddr);
void setComID(uint32_t comID);
void setInterval(uint32_t interval);

void setIPRec(int index, const char* ipAddr);
void setComIDRec(int index, uint32_t comID);

//int pd_loop();
int pd_loop2();
