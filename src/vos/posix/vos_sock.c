/**********************************************************************************************************************/
/**
 * @file            posix/vos_sock.c
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

#ifndef POSIX
#error \
    "You are trying to compile the POSIX implementation of vos_sock.c - either define POSIX or exclude this file!"
#endif

/***********************************************************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#ifdef __linux
#include <linux/if.h>
#include <linux/in.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#endif

#include "vos_utils.h"
#include "vos_sock.h"
#include "vos_thread.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

#define VOS_DEFAULT_IFACE  "eth0"

/***********************************************************************************************************************
 *  LOCALS
 */

BOOL            vosSockInitialised      = FALSE;
uint32_t        gNumberOfOpenSockets    = 0;
struct ifreq    gIfr;

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
 *  @retval             TRUE        address is multicast
 *  @retval             FALSE       address is not a multicast address
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
    sprintf(dotted, "%u.%u.%u.%u", ipAddress >> 24, (ipAddress >> 16) & 0xFF,
            (ipAddress >> 8) & 0xFF, ipAddress & 0xFF);
    return dotted;
}

/*	Sockets	*/

/**********************************************************************************************************************/
/** Initialize the socket library.
 *  Must be called once before any other call
 *
 *  @retval         VOS_NO_ERR          no error
 *  @retval         VOS_SOCK_ERR        sockets not supported
 */

EXT_DECL VOS_ERR_T vos_sockInit (void)
{
    memset(&gIfr, 0, sizeof(gIfr));
    vosSockInitialised = TRUE;

    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Return the MAC address of the default adapter.
 *
 *  @param[out]     pMAC            return MAC address.
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMAC == NULL
 *  @retval         VOS_SOCK_ERR    socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockGetMAC (
    UINT8 pMAC[6])
{
    int sock;
    int i;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pMAC == NULL)
    {
        vos_printf(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    /* Not every system supports this! */
#ifndef __APPLE__

    /* Has it been determined before? */
    for(i = 0; i < 6 && gIfr.ifr_hwaddr.sa_data[i] != 0; i++)
    {
        ;
    }

    if (i >= 6) /* needs to be determined */
    {

        if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
        {
            memset(pMAC, 0, sizeof(mac));
            vos_printf(VOS_LOG_ERROR, "socket() failed\n");
            return VOS_SOCK_ERR;
        }

        /* get the MAC address; we will construct our UUID from this (if UUID generation is not supported) */
        gIfr.ifr_addr.sa_family = AF_INET;
        strncpy(gIfr.ifr_name, VOS_DEFAULT_IFACE, IFNAMSIZ - 1);

        ioctl(sock, SIOCGIFHWADDR, &gIfr);

        close(sock);
    }

    for( i = 0; i < 6; i++ )
    {
        pMAC[i] = (UINT8) gIfr.ifr_hwaddr.sa_data[i];
    }
    return VOS_NO_ERR;
#else
    return VOS_SOCK_ERR;
#endif
}

/**********************************************************************************************************************/
/** Create an UDP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *  applied later.
 *  Note: Some targeted systems might not support every option.
 *
 *  @param[out]     pSock           pointer to socket descriptor returned
 *  @param[in]      pOptions        pointer to socket options (optional)
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pSock == NULL
 *  @retval         VOS_SOCK_ERR    socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockOpenUDP (
    INT32                   *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    int sock;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pSock == NULL)
    {
        vos_printf(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        vos_printf(VOS_LOG_ERROR, "socket call failed\n");
        return VOS_SOCK_ERR;
    }

    if (vos_sockSetOptions(sock, pOptions) != VOS_NO_ERR)
    {
        close(sock);
        return VOS_SOCK_ERR;
    }

    *pSock = (INT32) sock;
    gNumberOfOpenSockets++;
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Create a TCP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *  applied later.
 *
 *  @param[out]     pSock           pointer to socket descriptor returned
 *  @param[in]      pOptions        pointer to socket options (optional)
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pSock == NULL
 *  @retval         VOS_SOCK_ERR    socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockOpenTCP (
    INT32                   *pSock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    int sock;

    if (!vosSockInitialised)
    {
        return VOS_INIT_ERR;
    }

    if (pSock == NULL)
    {
        vos_printf(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        vos_printf(VOS_LOG_ERROR, "socket call failed\n");
        return VOS_SOCK_ERR;
    }

    if (vos_sockSetOptions(sock, pOptions) != VOS_NO_ERR)
    {
        close(sock);
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
 *  @param[in]      sock            socket descriptor
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown
 */

EXT_DECL VOS_ERR_T vos_sockClose (
    INT32 sock)
{
    if (close(sock) == -1)
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
 *  @param[in]      sock            socket descriptor
 *  @param[in]      pOptions        pointer to socket options (optional)
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown
 */

EXT_DECL VOS_ERR_T vos_sockSetOptions (
    INT32                   sock,
    const VOS_SOCK_OPT_T    *pOptions)
{
    int sockOptValue = 0;

    if (pOptions)
    {
        if (1 == pOptions->reuseAddrPort)
        {
            sockOptValue = 1;
#ifdef SO_REUSEPORT
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                vos_printf(VOS_LOG_WARNING, "Reuse ADDR & Port failed\n");
            }
#else
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                vos_printf(VOS_LOG_WARNING, "Reuse ADDR failed\n");
            }
#endif
        }
        if (1 == pOptions->nonBlocking)
        {
            if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
            {
                vos_printf(VOS_LOG_ERROR, "non blocking failed\n");
                return VOS_SOCK_ERR;
            }
        }
        if (pOptions->qos > 0 && pOptions->qos < 8)
        {
            /* The QoS value (0-7) is mapped to MSB bits 7-5, bit 2 is set for local use */
            sockOptValue = (int) ((pOptions->qos << 5) | 4);
            if (setsockopt(sock, IPPROTO_IP, IP_TOS, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                vos_printf(VOS_LOG_WARNING, "setsockopt IP_TOS/QOS failed\n");
            }
        }
        if (pOptions->ttl > 0)
        {
            sockOptValue = pOptions->ttl;
            if (setsockopt(sock, IPPROTO_IP, IP_TTL, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                vos_printf(VOS_LOG_WARNING, "setsockopt IP_TTL failed\n");
            }
        }
        if (pOptions->ttl_multicast > 0)
        {
            sockOptValue = pOptions->ttl_multicast;
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
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
 *  @param[in]      sock            socket descriptor
 *  @param[in]      mcAddress       multicast group to join
 *  @param[in]      ipAddress       depicts interface on which to join, default 0 for any
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_SOCK_ERR    option not supported
 */

EXT_DECL VOS_ERR_T vos_sockJoinMC (
    INT32   sock,
    UINT32  mcAddress,
    UINT32  ipAddress)
{
    struct ip_mreq  mreq;
    VOS_ERR_T       err = VOS_NO_ERR;

    if (sock == -1)
    {
        err = VOS_PARAM_ERR;
    }
    /* Is this a multicast address? */
    else if (IN_MULTICAST(mcAddress))
    {

        mreq.imr_multiaddr.s_addr   = vos_htonl(mcAddress);
        mreq.imr_interface.s_addr   = vos_htonl(ipAddress);

        vos_printf(VOS_LOG_INFO, "joining MC: %s on iface %s\n",
                   inet_ntoa(mreq.imr_multiaddr), inet_ntoa(mreq.imr_interface));

        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
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
            UINT32 enMcLb = 0;
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &enMcLb, sizeof(enMcLb)) == -1)
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
 *  @param[in]      sock            socket descriptor
 *  @param[in]      mcAddress       multicast group to join
 *  @param[in]      ipAddress       depicts interface on which to leave, default 0 for any
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_SOCK_ERR    option not supported
 */

EXT_DECL VOS_ERR_T vos_sockLeaveMC (
    INT32   sock,
    UINT32  mcAddress,
    UINT32  ipAddress)
{
    struct ip_mreq  mreq;
    VOS_ERR_T       err = VOS_NO_ERR;

    if (sock == -1)
    {
        err = VOS_PARAM_ERR;
    }
    /* Is this a multicast address? */
    else if (IN_MULTICAST(mcAddress))
    {

        mreq.imr_multiaddr.s_addr   = vos_htonl(mcAddress);
        mreq.imr_interface.s_addr   = vos_htonl(ipAddress);

        vos_printf(VOS_LOG_INFO, "leaving MC: %s on iface %s\n",
                   inet_ntoa(mreq.imr_multiaddr), inet_ntoa(mreq.imr_interface));


        if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
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
 *  @param[in]      sock            socket descriptor
 *  @param[in]      pBuffer         pointer to data to send
 *  @param[in]      size            size of the data to send
 *  @param[in]      ipAddress       destination IP
 *  @param[in]      port            destination port
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be sent
 *  @retval         VOS_MEM_ERR     resource error
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
    while (err == -1 && errno == EAGAIN);

    if (err == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_ERROR, "vos_sockSendUDP failed (%s)\n",
                   buff);
        return VOS_IO_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Receive UDP data.
 *  The caller must provide a sufficient sized buffer. If the supplied buffer is smaller than the bytes received, *pSize
 *  will reflect the number of copied bytes and the call should be repeated until *pSize is 0 (zero).
 *  If the socket was created in blocking-mode (default), then this call will block and will only return if data has
 *  been received or the socket was closed or an error occured.
 *  If called in non-blocking mode, and no data is available, VOS_NODATA_ERR will be returned.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[out]     pBuffer         pointer to applications data buffer
 *  @param[in,out]  pSize           pointer to the received data size
 *  @param[out]     pIPAddr         source IP
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be read
 *  @retval         VOS_NODATA_ERR  no data in non-blocking
 */

EXT_DECL VOS_ERR_T vos_sockReceiveUDP (
    INT32   sock,
    UINT8   *pBuffer,
    UINT32  *pSize,
    UINT32  *pIPAddr)
{
    struct sockaddr_in  srcAddr;
    socklen_t           sockLen = sizeof(srcAddr);
    ssize_t rcvSize = 0;

    memset(&srcAddr, 0, sizeof(srcAddr));

    rcvSize = recvfrom(sock,
                       pBuffer,
                       *pSize,
                       0,
                       (struct sockaddr *) &srcAddr,
                       &sockLen);

    *pIPAddr = (uint32_t) vos_ntohl(srcAddr.sin_addr.s_addr);
    /* vos_printf(VOS_LOG_INFO, "recvfrom found %d bytes for IP address %x\n", rcvSize, *pIPAddr); */

    if (rcvSize == -1 && errno == EAGAIN)
    {
        *pSize = 0;
        return VOS_NODATA_ERR;
    }
    else if (rcvSize == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
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
 *  @param[in]      sock            socket descriptor
 *  @param[in]      ipAddress       source IP to receive on, 0 for any
 *  @param[in]      port            port to receive on, 20548 for PD
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      Input/Output error
 *  @retval         VOS_MEM_ERR     resource error
 */

EXT_DECL VOS_ERR_T vos_sockBind (
    INT32   sock,
    UINT32  ipAddress,
    UINT16  port)
{
    struct sockaddr_in srcAddress;

    if (sock == -1)
    {
        return VOS_PARAM_ERR;
    }

    /* Allow the socket to be bound to an address and port
        that is already in use */

    memset((char *)&srcAddress, 0, sizeof(srcAddress));
    srcAddress.sin_family       = AF_INET;
    srcAddress.sin_addr.s_addr  = vos_htonl(ipAddress);
    srcAddress.sin_port         = vos_htons(port);

    vos_printf(VOS_LOG_INFO, "binding to: %s:%hu\n",
               inet_ntoa(srcAddress.sin_addr), port);

    /*  Try to bind the socket to the PD port. */
    if (bind(sock, (struct sockaddr *)&srcAddress, sizeof(srcAddress)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_ERROR, "bind failed(%s)\n", buff);
        return VOS_SOCK_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Listen for incoming connections.
 *
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      backlog	        maximum connection attempts if system is busy
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      Input/Output error
 *  @retval         VOS_MEM_ERR     resource error
 */

EXT_DECL VOS_ERR_T vos_sockListen (
    INT32   sock,
    UINT32  backlog)
{
    if (sock == -1)
    {
        return VOS_PARAM_ERR;
    }
    if (listen(sock, (int) backlog) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_ERROR, "listen failed (%s)\n", buff);
        return VOS_SOCK_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Accept an incoming TCP connection.
 *  Accept incoming connections on the provided socket. May block and will return a new socket descriptor when
 *  accepting a connection. The original socket *pSock, remains open.
 *
 *
 *  @param[in]      sock            Socket descriptor
 *  @param[out]     pSock           Pointer to socket descriptor, on exit new socket
 *  @param[out]     pIPAddress      source IP to receive on, 0 for any
 *  @param[out]     pPort           port to receive on, 20548 for PD
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   NULL parameter, parameter error
 *  @retval         VOS_UNKNOWN_ERR sock descriptor unknown error
 */

EXT_DECL VOS_ERR_T vos_sockAccept (
    INT32   sock,
    INT32   *pSock,
    UINT32  *pIPAddress,
    UINT16  *pPort)
{
    struct sockaddr_in srcAddress;
    int connFd = -1;

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
        socklen_t sockLen = sizeof(srcAddress);

        connFd = accept(sock, (struct sockaddr *) &srcAddress, &sockLen);
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
                    strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
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
 *  @param[in]      sock            socket descriptor
 *  @param[in]      ipAddress       destination IP
 *  @param[in]      port            destination port
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      Input/Output error
 *  @retval         VOS_MEM_ERR     resource error
 */

EXT_DECL VOS_ERR_T vos_sockConnect (
    INT32   sock,
    UINT32  ipAddress,
    UINT16  port)
{
    struct sockaddr_in dstAddress;

    if (sock == -1)
    {
        return VOS_PARAM_ERR;
    }

    memset((char *)&dstAddress, 0, sizeof(dstAddress));
    dstAddress.sin_family       = AF_INET;
    dstAddress.sin_addr.s_addr  = vos_htonl(ipAddress);
    dstAddress.sin_port         = vos_htons(port);

    if (connect(sock, (const struct sockaddr *) &dstAddress,
                sizeof(dstAddress)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_WARNING, "connect failed (%s)\n", buff);
        return VOS_IO_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Send TCP data.
 *  Send data to the supplied address and port.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      pBuffer         pointer to data to send
 *  @param[in]      size            size of the data to send
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be sent
 *  @retval         VOS_NODATA_ERR  no data was sent in non-blocking
 */

EXT_DECL VOS_ERR_T vos_sockSendTCP (
    INT32       sock,
    const UINT8 *pBuffer,
    UINT32      size)
{
    ssize_t sendSize    = 0;
    size_t  bufferSize  = (size_t) size;

    if (sock == -1)
    {
        return VOS_PARAM_ERR;
    }

    /* Keep on sending until we got rid of all data or we received an unrecoverable error */
    do
    {
        sendSize = write(sock, pBuffer, bufferSize);
        if (sendSize >= 0)
        {
            bufferSize  -= sendSize;
            pBuffer     += sendSize;
        }
        if(sendSize == -1 && errno == EWOULDBLOCK)
        {
            return VOS_NODATA_ERR;
        }
    }
    while (bufferSize && !(sendSize == -1 && errno != EINTR));

    if (sendSize == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_WARNING, "send failed (%s)\n", buff);
        return VOS_IO_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Receive TCP data.
 *  The caller must provide a sufficient sized buffer. If the supplied buffer is smaller than the bytes received, *pSize
 *  will reflect the number of copied bytes and the call should be repeated until *pSize is 0 (zero).
 *  If the socket was created in blocking-mode (default), then this call will block and will only return if data has
 *  been received or the socket was closed or an error occured.
 *  If called in non-blocking mode, and no data is available, VOS_NODATA_ERR will be returned.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[out]     pBuffer         pointer to applications data buffer
 *  @param[in,out]  pSize           pointer to the received data size
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be read
 *  @retval         VOS_NODATA_ERR  no data in non-blocking
 */

EXT_DECL VOS_ERR_T vos_sockReceiveTCP (
    INT32   sock,
    UINT8   *pBuffer,
    UINT32  *pSize)
{
    ssize_t rcvSize     = 0;
    size_t  bufferSize  = (size_t) *pSize;

    *pSize = 0;

    if (sock == -1 || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }

    do
    {

        rcvSize = read(sock, pBuffer, bufferSize);
        if (rcvSize > 0)
        {
            bufferSize  -= rcvSize;
            pBuffer     += rcvSize;
            *pSize      += rcvSize;
        }

        if(rcvSize == -1 && errno == EWOULDBLOCK)
        {
            if (*pSize == 0)
            {
                return VOS_NODATA_ERR;
            }
            else
            {
                return VOS_NO_ERR;
            }
        }
    }
    while ((bufferSize > 0 && rcvSize > 0) || (rcvSize == -1 && errno == EINTR));

    if (rcvSize == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_WARNING, "receive failed (%s)\n", buff);
        return VOS_IO_ERR;
    }
    else if (*pSize == 0)
    {
        return VOS_NODATA_ERR;
    }
    else
    {
        return VOS_NO_ERR;
    }
}

#ifdef TRDP_OPTION_LADDER
/**********************************************************************************************************************/
/** Get interface address
 *
 *  @param[in,out]  pIfa_list       pointer to pointer to the I/F all list
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be read
  */

EXT_DECL VOS_ERR_T vos_getIfAddrs (
    struct ifaddrs * *ppIfa_list)
{

    if (ppIfa_list == NULL)
    {
        return VOS_PARAM_ERR;
    }
    if (getifaddrs(ppIfa_list) != 0)
    {
        vos_printf(VOS_LOG_ERROR, "getifaddrs error. errno=%d\n", errno);
        return VOS_SOCK_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Clear interface address memory area
 *
 *  @param[in,out]  pIfa_list       pointer to the I/F all list
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
  */

EXT_DECL VOS_ERR_T vos_freeIfAddrs (
    struct ifaddrs *pIfa_list)
{
    if (pIfa_list == NULL)
    {
        return VOS_PARAM_ERR;
    }

    freeifaddrs(pIfa_list);
    return VOS_NO_ERR;
}

#endif /* TRDP_OPTION_LADDER */

/**********************************************************************************************************************/
/** Set Using Multicast I/F
 *
 *  @param[in]      sock                        socket descriptor
 *  @param[in]      usingMulticastIfAddress     using Multicast I/F Address
 *  @retval         VOS_NO_ERR                  no error
 *  @retval         VOS_PARAM_ERR               sock descriptor unknown, parameter error
 *  @retval         VOS_SOCK_ERR                option not supported
 */
EXT_DECL VOS_ERR_T vos_sockSetMulticastIf (
    INT32   sock,
    UINT32  usingMulticastIfAddress)
{
    struct sockaddr_in  multicastIFAddress;
    VOS_ERR_T           err = VOS_NO_ERR;

    if (sock == -1)
    {
        err = VOS_PARAM_ERR;
    }
    else
    {
        /* Multicast I/F setting */
        memset((char *)&multicastIFAddress, 0, sizeof(multicastIFAddress));
        multicastIFAddress.sin_addr.s_addr = vos_htonl(usingMulticastIfAddress);

        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &multicastIFAddress.sin_addr, sizeof(struct in_addr)) == -1)
        {
            vos_print(VOS_LOG_WARNING, "setsockopt IP_MULTICAST_IF failed\n");
            err = VOS_SOCK_ERR;
        }
        else
        {
            /*	DEBUG
                struct sockaddr_in myAddress = {0};
                socklen_t optionSize ;
                getsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &myAddress.sin_addr, &optionSize);
            */
            err = VOS_NO_ERR;
        }
    }
    return err;
}
