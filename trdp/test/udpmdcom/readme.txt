(20/11/2012 - s.pachera, FAR Systems s.p.a.)
UDPMDCom modification to TRDP repository rev. 158
	Created folder test/udpmdcom
	Added file test/udpmdcom/mdTest0001.c
	Modified file Makefile to support mdTest0001.c compilation
	Modified trdp_if.c, line 2870 become: appHandle->iface[pNewElement->socketIdx].usage == 0)
	Modified trdp_utils.c, line 460 added: sock_options.nonBlocking = TRUE;
	Modified trdp_mdcom.c, line 134 become: if(l_datasetLength > 0)
	Modified trdp_mdcom.c, line 247 become: if(pPacket->frameHead.datasetLength > 0)

To execute UDPMDCom test use two devices Dev1 (IP1) and Dev2 (Dev2).
On Dev2
	./mdTest0001 --testmode 2
On Dev1
	./mdTest0001 --testmode 1 --dest <IP2>
	A command line interface will be shown
		Type "h" and return to get available commands
		To execute test type related char and next return
		Result will be shown on Dev1 and Dev2 screen

