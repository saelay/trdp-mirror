/**********************************************************************************************************************/
/**
 * @file            windows/vos_sock.c
 *
 * @brief           Socket functions
 *
 * @details         OS abstraction of IP socket functions for UDP and TCP
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 *
 * @remarks         All rights reserved. Reproduction, modification, use or disclosure
 *                  to third parties without express authority is forbidden,
 *                  Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
 */

#ifndef WIN32
#error \
    "You are trying to compile the POSIX implementation of vos_sock.c - either define POSIX or exclude this file!"
#endif

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <lm.h>

#include "vos_utils.h"
#include "vos_sock.h"
#include "vos_thread.h"

#pragma comment(lib, "IPHLPAPI.lib")

/* include the needed windows network libraries */
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Netapi32.lib")

/***********************************************************************************************************************
 * DEFINITIONS
 */
#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

/***********************************************************************************************************************
 *  LOCALS
 */

BOOL    vosSockInitialised      = FALSE;
UINT32  gNumberOfOpenSockets    = 0;
UINT8   mac[6];

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Byte swapping.
 *
 *  @param[in]          val             Initial value.
 *  @retval             swapped value
 */

EXT_DECL UINT16 vos_htons (
    UINT16 val)
{
    return htons(val);
}

EXT_DECL UINT16 vos_ntohs (
    UINT16 val)
{
    return ntohs(val);
}

EXT_DECL UINT32 vos_htonl (
    UINT32 val)
{
    return htonl(val);
}

EXT_DECL UINT32 vos_ntohl (
    UINT32 val)
{
    return ntohl(val);
}

/**********************************************************************************************************************/
/** Check if the supplied address is a multicast group address.
 *
 *  @param[in]          ipAddress        IP address to check.
 *  @retval             TRUE		address is multicast
 *  @retval             FALSE		address is not a multicast address
 */

EXT_DECL BOOL vos_isMulticast (
    UINT32 ipAddress)
{
    return IN_MULTICAST(ipAddress);
}

/**********************************************************************************************************************/
/** Convert IP address.
 *
 *  @param[in]          pDottedIP   IP address as dotted decimal.
 *  @retval             address in UINT32 in host endianess
 */
EXT_DECL UINT32 vos_dottedIP (
    const CHAR8 *pDottedIP)
{
    return vos_ntohl(inet_addr(pDottedIP));
}

/**********************************************************************************************************************/
/** Convert IP address to dotted dec.
 *
 *  @param[in]          ipAddress   IP address as dotted decimal.
 *  @retval             address in UINT32 in host endianess
 */

EXT_DECL const CHAR8 *vos_ipDotted (
    UINT32 ipAddress)
{
    static CHAR8 dotted[16];
    sprintf_s (dotted, sizeof(dotted),"%u.%u.%u.%u", ipAddress >> 24, (ipAddress >> 16) & 0xFF,
            (ipAddress >> 8) & 0xFF, ipAddress & 0xFF);
    return dotted;
}

/*	Sockets	*/

/**********************************************************************************************************************/
/** Initialize the socket library.
 *  Must be called once before any other call
 *
 *  @retval         VOS_NO_ERR			no error
 *  @retval         VOS_SOCK_ERR		sockets not supported
 */

EXT_DECL VOS_ERR_T vos_sockInit (void)
{
    memset(mac, 0, sizeof(mac));
    vosSockInitialised = TRUE;

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the MAC address of the default adapter.
 *
 *  @param[out]     pMAC            return MAC address.
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	pMAC == NULL
 *  @retval         VOS_SOCK_ERR	socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockGetMAC (
    UINT8 pMAC[6])
{
    UINT32 i;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pMAC == NULL)
    {
        vos_printf(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    /*	Has it been determined before?	*/
    for(i = 0; (i < sizeof(mac)) && (mac[i] != 0); i++)
    {
        ;
    }

    if (i >= 6) /*	needs to be determined	*/
    {
        /* for NetBIOS */
        DWORD   dwEntriesRead;
        DWORD   dwTotalEntries;
        BYTE    *pbBuffer;
        WKSTA_TRANSPORT_INFO_0  *pwkti;

        /* Get MAC address via NetBIOS's enumerate function */
        NET_API_STATUS  dwStatus = NetWkstaTransportEnum(
                NULL,                     /* [in] server name */
                0,                        /* [in] data structure to return */
                &pbBuffer,                /* [out] pointer to buffer */
                MAX_PREFERRED_LENGTH,     /* [in] maximum length */
                &dwEntriesRead,           /* [out] counter of elements actually enumerated */
                &dwTotalEntries,          /* [out] total number of elements that could be enumerated */
                NULL);                    /* [in/out] resume handle */


        pwkti = (WKSTA_TRANSPORT_INFO_0 *)pbBuffer;

        /* first address is 00000000, skip it */
        swscanf_s(
            (wchar_t *)pwkti[1].wkti0_transport_address,
            L"%2hx%2hx%2hx%2hx%2hx%2hx",
            &mac[0],
            &mac[1],
            &mac[2],
            &mac[3],
            &mac[4],
            &mac[5]);

        /* Release pbBuffer allocated by NetWkstaTransportEnum */
        dwStatus = NetApiBufferFree(pbBuffer);
    }

    for( i = 0; i < sizeof(mac); i++ )
    {
        pMAC[i] = (UINT8) mac[i];
    }
    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Create an UDP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *	applied later.
 *	Note: Some targeted systems might not support every option.
 *
 *  @param[out]     pSock			pointer to socket descriptor returned
 *  @param[in]      pOptions		pointer to socket options (optional)
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	pSock == NULL
 *  @retval         VOS_SOCK_ERR	socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockOpenUDP (
    INT32                   *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    SOCKET  sock;
    WSADATA WsaDat;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pSock == NULL)
    {
        vos_printf(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    /* The windows socket library has to be prepared, before it could be used */
    if (WSAStartup(MAKEWORD(2, 2), &WsaDat) != 0)
    {
        vos_printf(VOS_LOG_ERROR, "Windows socket API failed\n");
        return VOS_SOCK_ERR;
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET )
    {
        vos_printf(VOS_LOG_ERROR, "socket call failed\n");
        return VOS_SOCK_ERR;
    }

    if (vos_sockSetOptions(sock, pOptions) != VOS_NO_ERR)
    {
        closesocket(sock);
        return VOS_SOCK_ERR;
    }

    *pSock = (INT32) sock;
    gNumberOfOpenSockets++;
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Create a TCP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *	applied later.
 *
 *  @param[out]     pSock			pointer to socket descriptor returned
 *  @param[in]      pOptions		pointer to socket options (optional)
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	pSock == NULL
 *  @retval         VOS_SOCK_ERR	socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockOpenTCP (
    INT32                   *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    SOCKET sock;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pSock == NULL)
    {
        vos_printf(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
    {
        vos_printf(VOS_LOG_ERROR, "socket call failed\n");
        return VOS_SOCK_ERR;
    }

    if (vos_sockSetOptions(sock, pOptions) != VOS_NO_ERR)
    {
        closesocket(sock);
        return VOS_SOCK_ERR;
    }
    *pSock = (INT32) sock;
    gNumberOfOpenSockets++;
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Close a socket.
 *  Release any resources aquired by this socket
 *
 *  @param[in]      sock			socket descriptor
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	sock descriptor unknown
 */

EXT_DECL VOS_ERR_T vos_sockClose (
    INT32 sock)
{
    if (closesocket(sock) == SOCKET_ERROR)
    {
        vos_printf(VOS_LOG_ERROR,
                   "vos_sockClose called with unknown descriptor\n");
        return VOS_PARAM_ERR;
    }
    gNumberOfOpenSockets--;
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Set socket options.
 *  Note: Some targeted systems might not support every option.
 *
 *  @param[in]      sock			socket descriptor
 *  @param[in]      pOptions		pointer to socket options (optional)
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	sock descriptor unknown
 */

EXT_DECL VOS_ERR_T vos_sockSetOptions (
    INT32                   sock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    int     sockOptValue = 0;
    u_long  value;

    if (pOptions)
    {
        if (1 == pOptions->reuseAddrPort)
        {
            sockOptValue = 1;
#ifdef SO_REUSEPORT
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &sockOptValue,
                           sizeof(sockOptValue)) == SOCKET_ERROR )
            {
                vos_printf(VOS_LOG_WARNING, "Reuse ADDR & Port failed\n");
            }
#else
            if (setsockopt((SOCKET)sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&sockOptValue,
                           sizeof(sockOptValue)) == SOCKET_ERROR )
            {
                vos_printf(VOS_LOG_WARNING, "Reuse ADDR failed\n");
            }
#endif
        }
        
        if (pOptions->nonBlocking == TRUE)
        {
            value = TRUE;
            
            if (ioctlsocket(sock, FIONBIO, &value) == SOCKET_ERROR)
            {
                vos_printf(VOS_LOG_ERROR, "non blocking failed\n");
                return VOS_SOCK_ERR;
            }
        }

        if (pOptions->qos > 0 && pOptions->qos < 8)
        {
            /* The QoS value (0-7) is mapped to MSB bits 7-5, bit 2 is set for local use */
            sockOptValue = (int) ((pOptions->qos << 5) | 4);
            if (setsockopt((SOCKET)sock, IPPROTO_IP, IP_TOS, (const char *)&sockOptValue,
                           sizeof(sockOptValue)) == SOCKET_ERROR )
            {
                vos_printf(VOS_LOG_WARNING, "setsockopt IP_TOS/QOS failed\n");
            }
        }
        if (pOptions->ttl > 0)
        {
            sockOptValue = pOptions->ttl;
            if (setsockopt((SOCKET)sock, IPPROTO_IP, IP_TTL, (const char *)&sockOptValue,
                           sizeof(sockOptValue)) == INVALID_SOCKET )
            {
                vos_printf(VOS_LOG_WARNING, "setsockopt IP_TTL failed\n");
            }
        }
        if (pOptions->ttl_multicast > 0)
        {
            sockOptValue = pOptions->ttl_multicast;
            if (setsockopt((SOCKET)sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&sockOptValue,
                           sizeof(sockOptValue)) == SOCKET_ERROR )
            {
                vos_printf(VOS_LOG_WARNING,
                           "setsockopt IP_MULTICAST_TTL failed\n");
            }
        }
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Join a multicast group.
 *  Note: Some targeted systems might not support this option.
 *
 *  @param[in]      sock			socket descriptor
 *  @param[in]      mcAddress		multicast group to join
 *  @param[in]      ipAddress		depicts interface on which to join, default 0 for any
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	sock descriptor unknown, parameter error
 *  @retval         VOS_SOCK_ERR	option not supported
 */

EXT_DECL VOS_ERR_T vos_sockJoinMC (
    INT32   sock,
    UINT32  mcAddress,
    UINT32  ipAddress)
{
    struct ip_mreq  mreq;
    VOS_ERR_T       err = VOS_NO_ERR;

    if (sock == INVALID_SOCKET )
    {
        err = VOS_PARAM_ERR;
    }
    /*	Is this a multicast address?	*/
    else if (IN_MULTICAST(mcAddress))
    {

        mreq.imr_multiaddr.s_addr   = vos_htonl(mcAddress);
        mreq.imr_interface.s_addr   = vos_htonl(ipAddress);

        vos_printf(VOS_LOG_INFO, "joining MC: %s\n",
                   inet_ntoa(mreq.imr_multiaddr));

        if (setsockopt((SOCKET)sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq, sizeof(mreq)) == SOCKET_ERROR)
        {
            vos_print(VOS_LOG_WARNING, "setsockopt IP_ADD_MEMBERSHIP failed\n");
            err = VOS_SOCK_ERR;
        }
        else
        {
            err = VOS_NO_ERR;
        }

        /* Disable multicast loop back */
        {
            mreq.imr_multiaddr.s_addr   = 0;
            mreq.imr_interface.s_addr   = 0;

            if (setsockopt((SOCKET)sock, IPPROTO_IP, IP_MULTICAST_LOOP, (const char *)&mreq, sizeof(mreq)) == SOCKET_ERROR)
            {
                vos_printf(VOS_LOG_WARNING, "setsockopt IP_MULTICAST_TTL failed\n");
                err = VOS_SOCK_ERR;
            }
            else
            {
                err = (err == VOS_SOCK_ERR) ? VOS_SOCK_ERR : VOS_NO_ERR;
            }
        }
    }
    else
    {
        err = VOS_PARAM_ERR;
    }

    return err;
}

/**********************************************************************************************************************/
/** Leave a multicast group.
 *  Note: Some targeted systems might not support this option.
 *
 *  @param[in]      sock			socket descriptor
 *  @param[in]      mcAddress		multicast group to join
 *  @param[in]      ipAddress		depicts interface on which to leave, default 0 for any
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	sock descriptor unknown, parameter error
 *  @retval         VOS_SOCK_ERR	option not supported
 */

EXT_DECL VOS_ERR_T vos_sockLeaveMC (
    INT32   sock,
    UINT32  mcAddress,
    UINT32  ipAddress)
{
    struct ip_mreq  mreq;
    VOS_ERR_T       err = VOS_NO_ERR;

    if (sock == INVALID_SOCKET )
    {
        err = VOS_PARAM_ERR;
    }
    /*	Is this a multicast address?	*/
    else if (IN_MULTICAST(mcAddress))
    {

        mreq.imr_multiaddr.s_addr   = vos_htonl(mcAddress);
        mreq.imr_interface.s_addr   = vos_htonl(ipAddress);

        vos_printf(VOS_LOG_INFO, "leaving MC: %s on iface %s\n",
                   inet_ntoa(mreq.imr_multiaddr), inet_ntoa(mreq.imr_interface));

        if (setsockopt((SOCKET)sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char *)&mreq,
                       sizeof(mreq)) == SOCKET_ERROR)
        {
            vos_print(VOS_LOG_WARNING, "setsockopt IP_DROP_MEMBERSHIP failed\n");
            err = VOS_SOCK_ERR;
        }
        else
        {
            err = VOS_NO_ERR;
        }
    }
    else
    {
        err = VOS_PARAM_ERR;
    }

    return err;
}

/**********************************************************************************************************************/
/** Send UDP data.
 *  Send data to the supplied address and port.
 *
 *  @param[in]      sock			socket descriptor
 *  @param[in]      pBuffer			pointer to data to send
 *  @param[in]      size			size of the data to send
 *  @param[in]      ipAddress		destination IP
 *  @param[in]      port			destination port
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR		data could not be sent
 *  @retval         VOS_MEM_ERR		resource error
 */

EXT_DECL VOS_ERR_T vos_sockSendUDP (
    INT32       sock,
    const UINT8 *pBuffer,
    UINT32      size,
    UINT32      ipAddress,
    UINT16      port)
{
    struct sockaddr_in destAddr;
    int err = 0;

    /*      We send UDP packets to the address  */
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family         = AF_INET;
    destAddr.sin_addr.s_addr    = vos_htonl(ipAddress);
    destAddr.sin_port           = vos_htons(port);

    vos_printf(VOS_LOG_DBG, "sending packet to %s:%u\n",
               inet_ntoa(destAddr.sin_addr), port);

    do
    {
        errno   = 0;
        err     = sendto(sock,
                         pBuffer,
                         size,
                         0,
                         (struct sockaddr *) &destAddr,
                         sizeof(destAddr));
    }
    while (err == SOCKET_ERROR && errno == EAGAIN);

    if (err == SOCKET_ERROR)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_s(buff, VOS_MAX_ERR_STR_SIZE, errno);
        vos_printf(VOS_LOG_ERROR, "vos_sockSendUDP failed (%s)\n", buff);
        return VOS_IO_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Receive UDP data.
 *  The caller must provide a sufficient sized buffer. If the supplied buffer is smaller than the bytes received, *pSize
 *	will reflect the number of copied bytes and the call should be repeated until *pSize is 0 (zero).
 *	If the socket was created in blocking-mode (default), then this call will block and will only return if data has
 *	been received or the socket was closed or an error occured.
 *	If called in non-blocking mode, and no data is available, VOS_NODATA_ERR will be returned.
 *
 *  @param[in]      sock			socket descriptor
 *  @param[out]     pBuffer			pointer to applications data buffer
 *  @param[in,out]  pSize			pointer to the received data size
 *  @param[out]     pIPAddr			source IP
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR		data could not be read
 *  @retval         VOS_NODATA_ERR	no data in non-blocking
 */

EXT_DECL VOS_ERR_T vos_sockReceiveUDP (
    INT32   sock,
    UINT8   *pBuffer,
    INT32   *pSize,
    UINT32  *pIPAddr)
{
    struct sockaddr_in srcAddr;
    int sockLen = sizeof(srcAddr);
    int rcvSize = 0;

    memset(&srcAddr, 0, sizeof(srcAddr));

    rcvSize = recvfrom(sock,
                       (char *) pBuffer,
                       *pSize,
                       0,
                       (SOCKADDR *) &srcAddr,
                       &sockLen);

    *pIPAddr = (UINT32) vos_ntohl(srcAddr.sin_addr.s_addr);
    /* vos_printf(VOS_LOG_INFO, "recvfrom found %d bytes for IP address %x\n", rcvSize, *pIPAddr); */

    if (rcvSize == SOCKET_ERROR && errno == EAGAIN)
    {
        *pSize = 0;
        return VOS_NODATA_ERR;
    }
    else if (rcvSize == SOCKET_ERROR)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_s(buff, VOS_MAX_ERR_STR_SIZE, errno);
        vos_printf(VOS_LOG_ERROR, "recvfrom failed (%s)\n", buff);
        *pSize = 0; 
        return VOS_IO_ERR;
    }

    *pSize = rcvSize;

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Bind a socket to an address and port.
 *
 *
 *  @param[in]      sock			socket descriptor
 *  @param[in]      ipAddress		source IP to receive on, 0 for any
 *  @param[in]      port			port to receive on, 20548 for PD
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR		Input/Output error
 *  @retval         VOS_MEM_ERR		resource error
 */

EXT_DECL VOS_ERR_T vos_sockBind (
    INT32   sock,
    UINT32  ipAddress,
    UINT16  port)
{
    struct sockaddr_in srcAddress;

    if (sock == INVALID_SOCKET )
    {
        return VOS_PARAM_ERR;
    }

    /*	Allow the socket to be bound to an address and port
        that is already in use	*/

    memset((char *)&srcAddress, 0, sizeof(srcAddress));
    srcAddress.sin_family       = AF_INET;
    srcAddress.sin_addr.s_addr  = vos_htonl(ipAddress);
    srcAddress.sin_port         = vos_htons(port);

    vos_printf(VOS_LOG_INFO, "binding to: %s:%hu\n",
               inet_ntoa(srcAddress.sin_addr), port);

    /*  Try to bind the socket to the PD port.	*/
    if (bind(sock, (struct sockaddr *)&srcAddress, sizeof(srcAddress)) == SOCKET_ERROR)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_s(buff, VOS_MAX_ERR_STR_SIZE, errno);
        vos_printf(VOS_LOG_ERROR, "bind failed(%s)\n", buff);
        return VOS_SOCK_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Listen for incoming connections.
 *
 *
 *  @param[in]      sock			socket descriptor
 *  @param[in]      backlog			maximum connection attempts if system is busy
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR		Input/Output error
 *  @retval         VOS_MEM_ERR		resource error
 */

EXT_DECL VOS_ERR_T vos_sockListen (
    INT32   sock,
    UINT32  backlog)
{
    if (sock == INVALID_SOCKET )
    {
        return VOS_PARAM_ERR;
    }
    if (listen(sock, (int) backlog) == SOCKET_ERROR)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_s(buff, VOS_MAX_ERR_STR_SIZE, errno);
        vos_printf(VOS_LOG_ERROR, "listen failed (%s)\n", buff);
        return VOS_SOCK_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Accept an incoming TCP connection.
 *  Accept incoming connections on the provided socket. May block and will return a new socket descriptor when
 *	accepting a connection. The original socket *pSock, remains open.
 *
 *
 *  @param[in]      sock			Socket descriptor
 *  @param[out]     pSock			Pointer to socket descriptor, on exit new socket
 *  @param[out]     pIPAddress		source IP to receive on, 0 for any
 *  @param[out]     pPort			port to receive on, 20548 for PD
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	NULL parameter, parameter error
 *  @retval         VOS_UNKNOWN_ERR	sock descriptor unknown error
 */

EXT_DECL VOS_ERR_T vos_sockAccept (
    INT32   sock,
    INT32   *pSock,
    UINT32  *pIPAddress,
    UINT16  *pPort)
{
    struct sockaddr_in srcAddress;
    int connFd = SOCKET_ERROR;

    if (pSock == NULL || pIPAddress == NULL || pPort == NULL)
    {
        return VOS_PARAM_ERR;
    }

    memset((char *)&srcAddress, 0, sizeof(srcAddress));

    srcAddress.sin_family       = AF_INET;
    srcAddress.sin_addr.s_addr  = vos_htonl(*pIPAddress);
    srcAddress.sin_port         = vos_htons(*pPort);

    for (;; )
    {
        int sockLen = sizeof(srcAddress);
        connFd = accept(*pSock, (struct sockaddr *) &srcAddress, &sockLen);
        if (connFd < 0)
        {
            switch (errno)
            {
                /*Accept return -1 and errno = EWOULDBLOCK,
                when there is no more connection requests.*/
                case EWOULDBLOCK:
                {
                    *pSock = connFd;
                    return VOS_NO_ERR;
                }
                case EINTR:         break;
                case ECONNABORTED:  break;
#if defined (EPROTO)
                case EPROTO:        break;
#endif
                default:
                {
                    char buff[VOS_MAX_ERR_STR_SIZE];
                    strerror_s(buff, VOS_MAX_ERR_STR_SIZE, errno);
                    vos_printf(VOS_LOG_ERROR,
                               "Error calling accept listenFd(%d): %s",
                               *pSock,
                               buff);
                    return VOS_UNKNOWN_ERR;
                }
            }
        }
        else
        {
            *pIPAddress = vos_htonl(srcAddress.sin_addr.s_addr);
            *pPort      = vos_htons(srcAddress.sin_port);
            *pSock      = connFd;
            break;         /* success */
        }
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Open a TCP connection.
 *
 *
 *  @param[in]      sock			socket descriptor
 *  @param[in]      ipAddress		destination IP
 *  @param[in]      port			destination port
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR		Input/Output error
 *  @retval         VOS_MEM_ERR		resource error
 */

EXT_DECL VOS_ERR_T vos_sockConnect (
    INT32   sock,
    UINT32  ipAddress,
    UINT16  port)
{
    struct sockaddr_in dstAddress;

    if (sock == INVALID_SOCKET )
    {
        return VOS_PARAM_ERR;
    }

    memset((char *)&dstAddress, 0, sizeof(dstAddress));
    dstAddress.sin_family       = AF_INET;
    dstAddress.sin_addr.s_addr  = vos_htonl(ipAddress);
    dstAddress.sin_port         = vos_htons(port);

    if (connect((SOCKET)sock, (const struct sockaddr *) &dstAddress,
                sizeof(dstAddress)) == SOCKET_ERROR)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_s(buff, VOS_MAX_ERR_STR_SIZE, errno);
        vos_printf(VOS_LOG_WARNING, "connect failed (%s)\n", buff);
        return VOS_IO_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Send TCP data.
 *  Send data to the supplied address and port.
 *
 *  @param[in]      sock			socket descriptor
 *  @param[in]      pBuffer			pointer to data to send
 *  @param[in]      size			size of the data to send
 *  @retval         VOS_NO_ERR		no error
 *  @retval         VOS_PARAM_ERR	sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR		data could not be sent
 */

EXT_DECL VOS_ERR_T vos_sockSendTCP (
    INT32       sock,
    const UINT8 *pBuffer,
    UINT32      size)
{
    int sendSize    = 0;
    int bufferSize  = (int) size;

    if (sock == INVALID_SOCKET )
    {
        return VOS_PARAM_ERR;
    }

    /*	Keep on sending until we got rid of all data or we received an unrecoverable error	*/
    do
    {
        sendSize = send((SOCKET)sock, pBuffer, bufferSize, 0);
        if (sendSize >= 0)
        {
            bufferSize  -= sendSize;
            pBuffer     += sendSize;
        }

    }
    while (bufferSize || (sendSize == SOCKET_ERROR && errno == EAGAIN));

    if (sendSize == SOCKET_ERROR)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_s(buff, VOS_MAX_ERR_STR_SIZE, errno);
        vos_printf(VOS_LOG_WARNING, "send failed (%s)\n", buff);
        return VOS_IO_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Receive TCP data.
 *  The caller must provide a sufficient sized buffer. If the supplied buffer is smaller than the bytes received, *pSize
 *	will reflect the number of copied bytes and the call should be repeated until *pSize is 0 (zero).
 *	If the socket was created in blocking-mode (default), then this call will block and will only return if data has
 *	been received or the socket was closed or an error occured.
 *	If called in non-blocking mode, and no data is available, VOS_NODATA_ERR will be returned.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[out]     pBuffer         pointer to applications data buffer
 *  @param[in,out]  pSize           pointer to the received data size
 *  @param[in]      blocking        blocking mode
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be read
 *  @retval         VOS_NODATA_ERR  no data in non-blocking
 */

EXT_DECL VOS_ERR_T vos_sockReceiveTCP (
    INT32   sock,
    UINT8   *pBuffer,
    INT32   *pSize)
{
    int             rcvSize    = 0;
    int             bufferSize = (size_t) *pSize;
    VOS_SOCK_OPT_T  options    = {0};

    *pSize = 0;
    
    if (sock == INVALID_SOCKET || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }
    
       /*	Keep on sending until we got rid of all data or we received an unrecoverable error	*/
    do
    {
        rcvSize = recv((SOCKET)sock, pBuffer, bufferSize, 0);
        if (rcvSize > 0)
        {
            bufferSize  -= rcvSize;
            pBuffer     += rcvSize;
            *pSize      += rcvSize;
        }
        
        if((rcvSize == -1) && (errno == EWOULDBLOCK))
        {
        	return VOS_NO_ERR;
        }
        
        if (rcvSize == 0)
        {
            return VOS_NODATA_ERR;
        }
    }
    while (bufferSize && (rcvSize == -1 && errno == EAGAIN));

    if (rcvSize == SOCKET_ERROR)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_s(buff, VOS_MAX_ERR_STR_SIZE, errno);
        vos_printf(VOS_LOG_WARNING, "receive failed (%s)\n", buff);
        return VOS_IO_ERR;
    }
    return VOS_NO_ERR;
}
