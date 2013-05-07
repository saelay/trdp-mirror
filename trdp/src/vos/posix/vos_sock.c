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
 *                  Copyright Bombardier Transportation GmbH, Germany, 2012-2013.
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
#   include <linux/if.h>
#else
#   include <net/if.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <ifaddrs.h>

#include "vos_utils.h"
#include "vos_sock.h"
#include "vos_thread.h"

/***********************************************************************************************************************
 * DEFINITIONS
 */

#ifndef VOS_DEFAULT_IFACE
#   if defined(__APPLE__) || defined(__QNXNTO__)
#       define VOS_DEFAULT_IFACE    "en0"
#   else
#       define VOS_DEFAULT_IFACE    "eth0"
#   endif
#endif

/***********************************************************************************************************************
 *  LOCALS
 */

BOOL            vosSockInitialised      = FALSE;
uint32_t        gNumberOfOpenSockets    = 0;
struct ifreq    gIfr;

/***********************************************************************************************************************
 * LOCAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Get the MAC address for a named interface.
 *
 *  @param[out]         pMacAddr   pointer to array of MAC address to return
 *  @param[in]          pIfName    pointer to interface name
 *
 *  @retval             TRUE if successfull
 */

BOOL vos_getMacAddress (
    UINT8       *pMacAddr,
    const char  *pIfName)
{
#ifdef __linux
    struct ifreq    ifinfo;
    int             sd;
    int             result = -1;

    if (pIfName == NULL)
    {
        strcpy(ifinfo.ifr_name, VOS_DEFAULT_IFACE);
    }
    else
    {
        strcpy(ifinfo.ifr_name, pIfName);
    }
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd != -1)
    {
        result = ioctl(sd, SIOCGIFHWADDR, &ifinfo);
        close(sd);
    }
    if ((result == 0) && (ifinfo.ifr_hwaddr.sa_family == 1))
    {
        memcpy(pMacAddr, ifinfo.ifr_hwaddr.sa_data, IFHWADDRLEN);
        return TRUE;
    }
    else
    {
        return FALSE;
    }

#elif defined(__APPLE__)
#   define LLADDR_OFF  11

    struct ifaddrs  *pIfList;
    BOOL found = FALSE;
    const char      *pName = VOS_DEFAULT_IFACE;

    if (pIfName != NULL)
    {
        pName = pIfName;
    }

    if (getifaddrs(&pIfList) == 0)
    {
        struct ifaddrs *cur;

        for (cur = pIfList; cur; cur = cur->ifa_next)
        {
            if ((cur->ifa_addr->sa_family == AF_LINK) &&
                (strcmp(cur->ifa_name, pName) == 0) &&
                cur->ifa_addr)
            {
                UINT8 *p = (UINT8 *)cur->ifa_addr;
                memcpy(pMacAddr, p + LLADDR_OFF, VOS_MAC_SIZE);
                found = TRUE;
                break;
            }
        }

        freeifaddrs(pIfList);
    }

    return found;

#elif defined(__QNXNTO__)
#   warning "no definition for get_mac_address() on this platform!"
#else
#   error no definition for get_mac_address() on this platform!
#endif
}

/***********************************************************************************************************************
 * GLOBAL FUNCTIONS
 */

/**********************************************************************************************************************/
/** Byte swapping.
 *
 *  @param[in]          val             Initial value.
 *
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
/** Convert IP address from dotted dec. to !host! endianess
 *
 *  @param[in]          pDottedIP     IP address as dotted decimal.
 *
 *  @retval             address in UINT32 in host endianess
 */
EXT_DECL UINT32 vos_dottedIP (
    const CHAR8 *pDottedIP)
{
    return vos_ntohl(inet_addr(pDottedIP));
}

/**********************************************************************************************************************/
/** Convert IP address to dotted dec. from !host! endianess.
 *
 *  @param[in]          ipAddress   address in UINT32 in host endianess
 *
 *  @retval             IP address as dotted decimal.
 */

EXT_DECL const CHAR8 *vos_ipDotted (
    UINT32 ipAddress)
{
    static CHAR8 dotted[16];

    snprintf(dotted, sizeof(dotted), "%u.%u.%u.%u", (ipAddress >> 24), ((ipAddress >> 16) & 0xFF),
             ((ipAddress >> 8) & 0xFF), (ipAddress & 0xFF));
    /*
       vos_snprintf(dotted, sizeof(dotted), "%u.%u.%u.%u", (ipAddress >> 24), ((ipAddress >> 16) & 0xFF),
              ((ipAddress >> 8) & 0xFF), (ipAddress & 0xFF));
     */
    return dotted;
}

/**********************************************************************************************************************/
/** Check if the supplied address is a multicast group address.
 *
 *  @param[in]          ipAddress   IP address to check.
 *
 *  @retval             TRUE        address is multicast
 *  @retval             FALSE       address is not a multicast address
 */

EXT_DECL BOOL vos_isMulticast (
    UINT32 ipAddress)
{
    return IN_MULTICAST(ipAddress);
}


/**********************************************************************************************************************/
/** select function.
 *  Set the ready sockets in the supplied sets.
 *    Note: Some target systems might define this function as NOP.
 *
 *  @param[in]      highDesc          max. socket descriptor + 1
 *  @param[in,out]  pReadableFD       pointer to readable socket set
 *  @param[in,out]  pWriteableFD      pointer to writeable socket set
 *  @param[in,out]  pErrorFD          pointer to error socket set
 *  @param[in]      pTimeOut          pointer to time out value
 *
 *  @retval         number of ready file descriptors
 */

EXT_DECL INT32 vos_select (
    INT32       highDesc,
    VOS_FDS_T   *pReadableFD,
    VOS_FDS_T   *pWriteableFD,
    VOS_FDS_T   *pErrorFD,
    VOS_TIME_T  *pTimeOut)
{
    return select(highDesc, (fd_set *) pReadableFD, (fd_set *) pWriteableFD,
                  (fd_set *) pErrorFD, (struct timeval *) pTimeOut);
}

/**********************************************************************************************************************/
/** Get a list of interface addresses
 *  The caller has to provide an array of interface records to be filled.
 *
 *  @param[in,out]  pAddrCnt          in:   pointer to array size of interface record
 *                                    out:  pointer to number of interface records read
 *  @param[in,out]  ifAddrs           array of interface records
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMAC == NULL
 */
EXT_DECL VOS_ERR_T vos_getInterfaces (
    UINT32          *pAddrCnt,
    VOS_IF_REC_T    ifAddrs[])
{
    int success;
    struct ifaddrs *addrs;
    struct ifaddrs *cursor;
    int count = 0;

    if (pAddrCnt == NULL ||
        *pAddrCnt == 0 ||
        ifAddrs == NULL)
    {
        return VOS_PARAM_ERR;
    }

    success = getifaddrs(&addrs) == 0;
    if (success)
    {
        cursor = addrs;
        while (cursor != 0 && count < *pAddrCnt)
        {
            if (cursor->ifa_addr != NULL && cursor->ifa_addr->sa_family == AF_INET)
            {
                ifAddrs[count].ipAddr   = ntohl(*(UINT32 *)&cursor->ifa_addr->sa_data[2]);
                ifAddrs[count].netMask  = ntohl(*(UINT32 *)&cursor->ifa_netmask->sa_data[2]);
                if (cursor->ifa_name != NULL)
                {
                    strncpy((char *) ifAddrs[count].name, cursor->ifa_name, VOS_MAX_IF_NAME_SIZE);
                    ifAddrs[count].name[VOS_MAX_IF_NAME_SIZE - 1] = 0;
                }
                vos_printf(VOS_LOG_INFO, "IP-Addr for '%s': %u.%u.%u.%u\n",
                           ifAddrs[count].name,
                           (ifAddrs[count].ipAddr >> 24) & 0xFF,
                           (ifAddrs[count].ipAddr >> 16) & 0xFF,
                           (ifAddrs[count].ipAddr >> 8)  & 0xFF,
                           ifAddrs[count].ipAddr        & 0xFF);

                if (vos_getMacAddress(ifAddrs[count].mac, ifAddrs[count].name) == TRUE)
                {
                    vos_printf(VOS_LOG_INFO, "Mac-Addr for '%s': %02x:%02x:%02x:%02x:%02x:%02x\n",
                               ifAddrs[count].name,
                               ifAddrs[count].mac[0],
                               ifAddrs[count].mac[1],
                               ifAddrs[count].mac[2],
                               ifAddrs[count].mac[3],
                               ifAddrs[count].mac[4],
                               ifAddrs[count].mac[5]);
                }
                count++;
            }
            cursor = cursor->ifa_next;
        }

        freeifaddrs(addrs);
    }
    else
    {
        return VOS_PARAM_ERR;
    }

    *pAddrCnt = count;
    return VOS_NO_ERR;
}

/*    Sockets    */

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
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMAC == NULL
 *  @retval         VOS_SOCK_ERR    socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockGetMAC (
    UINT8 pMAC[VOS_MAC_SIZE])
{
    if (pMAC == NULL)
    {
        vos_printf(VOS_LOG_ERROR, "Parameter error");
        return VOS_PARAM_ERR;
    }

    if (vos_getMacAddress(pMAC, VOS_DEFAULT_IFACE) == TRUE)
    {
        return VOS_NO_ERR;
    }

    return VOS_SOCK_ERR;
}

/**********************************************************************************************************************/
/** Create an UDP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *  applied later.
 *  Note: Some targeted systems might not support every option.
 *
 *  @param[out]     pSock           pointer to socket descriptor returned
 *  @param[in]      pOptions        pointer to socket options (optional)
 *
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
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_ERROR, "socket() failed (Err: %s)\n", buff);
        return VOS_SOCK_ERR;
    }

    if (vos_sockSetOptions(sock, pOptions) != VOS_NO_ERR)
    {
        close(sock);
        return VOS_SOCK_ERR;
    }

    *pSock = (INT32) sock;
    gNumberOfOpenSockets++;
    vos_printf(VOS_LOG_INFO, "vos_sockOpenUDP: socket()=%d success\n", sock);
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Create a TCP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *  applied later.
 *
 *  @param[out]     pSock           pointer to socket descriptor returned
 *  @param[in]      pOptions        pointer to socket options (optional)
 *
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
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_ERROR, "socket() failed (Err: %s)\n", buff);
        return VOS_SOCK_ERR;
    }

    if (vos_sockSetOptions(sock, pOptions) != VOS_NO_ERR)
    {
        close(sock);
        return VOS_SOCK_ERR;
    }
    *pSock = (INT32) sock;
    gNumberOfOpenSockets++;
    vos_printf(VOS_LOG_INFO, "vos_sockOpenTCP: socket()=%d success\n", sock);
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
                   "vos_sockClose(%d) called with unknown descriptor\n",sock);
        return VOS_PARAM_ERR;
    }
    else
    {
        vos_printf(VOS_LOG_INFO,
                   "vos_sockClose(%d) okay\n",sock);
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
 *
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
            errno = 0;
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
                vos_printf(VOS_LOG_ERROR, "setsockopt() SO_REUSEPORT failed (Err: %s)\n", buff);
            }
#else
            errno = 0;
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
                vos_printf(VOS_LOG_ERROR, "setsockopt() SO_REUSEADDR failed (Err: %s)\n", buff);
            }
#endif
        }
        if (1 == pOptions->nonBlocking)
        {
            errno = 0;
            if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
                vos_printf(VOS_LOG_ERROR, "setsockopt() O_NONBLOCK failed (Err: %s)\n", buff);
                return VOS_SOCK_ERR;
            }
        }
        if (pOptions->qos > 0 && pOptions->qos < 8)
        {
            /* The QoS value (0-7) is mapped to MSB bits 7-5, bit 2 is set for local use */
            errno           = 0;
            sockOptValue    = (int) ((pOptions->qos << 5) | 4);
            if (setsockopt(sock, IPPROTO_IP, IP_TOS, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
                vos_printf(VOS_LOG_ERROR, "setsockopt() IP_TOS failed (Err: %s)\n", buff);
            }
        }
        if (pOptions->ttl > 0)
        {
            errno           = 0;
            sockOptValue    = pOptions->ttl;
            if (setsockopt(sock, IPPROTO_IP, IP_TTL, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
                vos_printf(VOS_LOG_ERROR, "setsockopt() IP_TTL failed (Err: %s)\n", buff);
            }
        }
        if (pOptions->ttl_multicast > 0)
        {
            errno           = 0;
            sockOptValue    = pOptions->ttl_multicast;
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &sockOptValue,
                           sizeof(sockOptValue)) == -1)
            {
                char buff[VOS_MAX_ERR_STR_SIZE];
                strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
                vos_printf(VOS_LOG_ERROR, "setsockopt() IP_MULTICAST_TTL failed (Err: %s)\n", buff);
            }
        }
    }
    /*  Include struct in_pktinfo in the message "ancilliary" control data.
        This way we can get the destination IP address for received UDP packets */
    errno           = 0;
    sockOptValue    = 1;
    #if defined(IP_RECVDSTADDR)
    if (setsockopt(sock, IPPROTO_IP, IP_RECVDSTADDR, &sockOptValue, sizeof(sockOptValue)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_ERROR, "setsockopt() IP_RECVDSTADDR failed (Err: %s)\n", buff);
    }
    #elif defined(IP_PKTINFO)
    if (setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &sockOptValue, sizeof(sockOptValue)) == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_ERROR, "setsockopt() IP_PKTINFO failed (Err: %s)\n", buff);
    }
    #endif

    return VOS_NO_ERR;
}


/**********************************************************************************************************************/
/** Join a multicast group.
 *  Note: Some targeted systems might not support this option.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      mcAddress       multicast group to join
 *  @param[in]      ipAddress       depicts interface on which to join, default 0 for any
 *
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
    VOS_ERR_T       result = VOS_NO_ERR;
    char buff[VOS_MAX_ERR_STR_SIZE];

    if (sock == -1)
    {
        result = VOS_PARAM_ERR;
    }
    /* Is this a multicast address? */
    else if (IN_MULTICAST(mcAddress))
    {
        mreq.imr_multiaddr.s_addr   = vos_htonl(mcAddress);
        mreq.imr_interface.s_addr   = vos_htonl(ipAddress);

        {
            char    mcStr[16];
            char    ifStr[16];

            strncpy(mcStr, inet_ntoa(mreq.imr_multiaddr), sizeof(mcStr));
            mcStr[sizeof(mcStr) - 1] = 0;
            strncpy(ifStr, inet_ntoa(mreq.imr_interface), sizeof(ifStr));
            ifStr[sizeof(ifStr) - 1] = 0;

            vos_printf(VOS_LOG_INFO, "joining MC: %s on iface %s\n", mcStr, ifStr);
        }

        errno = 0;
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1 &&
            errno != EADDRINUSE)
        {
            strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
            vos_printf(VOS_LOG_ERROR, "setsockopt() IP_ADD_MEMBERSHIP failed (Err: %s)\n", buff);
            result = VOS_SOCK_ERR;
        }
        else
        {
            result = VOS_NO_ERR;
        }

        /* Disable multicast loop back */
        /* only useful for streaming
        {
            UINT32 enMcLb = 0;

            errno = 0;
            if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &enMcLb, sizeof(enMcLb)) == -1
                 && errno != 0)
            {
                strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
                vos_printf(VOS_LOG_ERROR, "setsockopt() IP_MULTICAST_LOOP failed (%s)\n", buff);
                result = VOS_SOCK_ERR;
            }
            else
            {
                result = (result == VOS_SOCK_ERR) ? VOS_SOCK_ERR : VOS_NO_ERR;
            }
        }
        */
    }
    else
    {
        result = VOS_PARAM_ERR;
    }

    return result;
}

/**********************************************************************************************************************/
/** Leave a multicast group.
 *  Note: Some targeted systems might not support this option.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      mcAddress       multicast group to join
 *  @param[in]      ipAddress       depicts interface on which to leave, default 0 for any
 *
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
    VOS_ERR_T       result = VOS_NO_ERR;

    if (sock == -1)
    {
        result = VOS_PARAM_ERR;
    }
    /* Is this a multicast address? */
    else if (IN_MULTICAST(mcAddress))
    {
        mreq.imr_multiaddr.s_addr   = vos_htonl(mcAddress);
        mreq.imr_interface.s_addr   = vos_htonl(ipAddress);

        {
            char    mcStr[16];
            char    ifStr[16];

            strncpy(mcStr, inet_ntoa(mreq.imr_multiaddr), sizeof(mcStr));
            mcStr[sizeof(mcStr) - 1] = 0;
            strncpy(ifStr, inet_ntoa(mreq.imr_interface), sizeof(ifStr));
            ifStr[sizeof(ifStr) - 1] = 0;

            vos_printf(VOS_LOG_INFO, "leaving MC: %s on iface %s\n", mcStr, ifStr);
        }

        errno = 0;
        if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
            vos_printf(VOS_LOG_ERROR, "setsockopt() IP_DROP_MEMBERSHIP failed (Err: %s)\n", buff);

            result = VOS_SOCK_ERR;
        }
        else
        {
            result = VOS_NO_ERR;
        }
    }
    else
    {
        result = VOS_PARAM_ERR;
    }

    return result;
}

/**********************************************************************************************************************/
/** Send UDP data.
 *  Send data to the supplied address and port.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      pBuffer         pointer to data to send
 *  @param[in,out]  pSize           In: size of the data to send, Out: no of bytes sent
 *  @param[in]      ipAddress       destination IP
 *  @param[in]      port            destination port
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be sent
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockSendUDP (
    INT32       sock,
    const UINT8 *pBuffer,
    UINT32      *pSize,
    UINT32      ipAddress,
    UINT16      port)
{
    struct sockaddr_in destAddr;
    ssize_t sendSize    = 0;
    size_t  size        = 0;

    if (sock == -1 || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }

    size    = *pSize;
    *pSize  = 0;

    /*      We send UDP packets to the address  */
    memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family         = AF_INET;
    destAddr.sin_addr.s_addr    = vos_htonl(ipAddress);
    destAddr.sin_port           = vos_htons(port);

    do
    {
        errno       = 0;
        sendSize    = sendto(sock,
                             (const char *)pBuffer,
                             size,
                             0,
                             (struct sockaddr *) &destAddr,
                             sizeof(destAddr));

        if (sendSize >= 0)
        {
            *pSize += sendSize;
        }

        if (sendSize == -1 && errno == EWOULDBLOCK)
        {
            return VOS_BLOCK_ERR;
        }
    }
    while (sendSize == -1 && errno != EINTR);

    if (sendSize == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_ERROR, "sendto() to %s:%u failed (Err: %s)\n",
                   inet_ntoa(destAddr.sin_addr), port, buff);
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
 *  If pointers are provided, source IP, source port and destination IP will be reported on return.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[out]     pBuffer         pointer to applications data buffer
 *  @param[in,out]  pSize           pointer to the received data size
 *  @param[out]     pSrcIPAddr      pointer to source IP
 *  @param[out]     pSrcIPPort      pointer to source port
 *  @param[out]     pDstIPAddr      pointer to dest IP
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be read
 *  @retval         VOS_NODATA_ERR  no data
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockReceiveUDP (
    INT32   sock,
    UINT8   *pBuffer,
    UINT32  *pSize,
    UINT32  *pSrcIPAddr,
    UINT16  *pSrcIPPort,
    UINT32  *pDstIPAddr)
{
    union
    {
        struct cmsghdr  cm;
        char            raw[32];
    } control_un;
    struct sockaddr_in  srcAddr;
    socklen_t           sockLen = sizeof(srcAddr);
    ssize_t rcvSize = 0;
    struct msghdr       msg;
    struct iovec        iov;
    struct cmsghdr      *cmsg;

    if (sock == -1 || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }

    /* clear our address buffers */
    memset(&msg, 0, sizeof(msg));
    memset(&control_un, 0, sizeof(control_un));

    /* fill the scatter/gather list with our data buffer */
    iov.iov_base    = pBuffer;
    iov.iov_len     = *pSize;

    /* fill the msg block for recvmsg */
    msg.msg_iov         = &iov;
    msg.msg_iovlen      = 1;
    msg.msg_name        = &srcAddr;
    msg.msg_namelen     = sockLen;
    msg.msg_control     = &control_un.cm;
    msg.msg_controllen  = sizeof(control_un);

    do
    {
        rcvSize = recvmsg(sock, &msg, 0);

        if (rcvSize != -1)
        {
            if (pDstIPAddr != NULL)
            {
                for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg))
                {
                    #if defined(IP_RECVDSTADDR)
                    if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVDSTADDR)
                    {
                        struct in_addr *pia = (struct in_addr *)CMSG_DATA(cmsg);
                        *pDstIPAddr = (UINT32)vos_ntohl(pia->s_addr);
                        /* vos_printf(VOS_LOG_DBG, "udp message dest IP: %s\n", vos_ipDotted(*pDstIPAddr)); */
                    }
                    #elif defined(IP_PKTINFO)
                    if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_PKTINFO)
                    {
                        struct in_pktinfo *pia = (struct in_pktinfo *)CMSG_DATA(cmsg);
                        *pDstIPAddr = (UINT32)vos_ntohl(pia->ipi_addr.s_addr);
                        /* vos_printf(VOS_LOG_DBG, "udp message dest IP: %s\n", vos_ipDotted(*pDstIPAddr)); */
                    }
                    #endif
                }
            }


            if (pSrcIPAddr != NULL)
            {
                *pSrcIPAddr = (uint32_t) vos_ntohl(srcAddr.sin_addr.s_addr);
                /* vos_printf(VOS_LOG_DBG, "udp message source IP: %s\n", vos_ipDotted(*pSrcIPAddr)); */
            }

            if (pSrcIPPort != NULL)
            {
                *pSrcIPPort = (UINT16) vos_ntohs(srcAddr.sin_port);
            }
        }

        if (rcvSize == -1 && errno == EWOULDBLOCK)
        {
            return VOS_BLOCK_ERR;
        }
    }
    while (rcvSize == -1 && errno == EINTR);

    *pSize = 0;

    if (rcvSize == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_ERROR, "recvmsg() failed (Err: %s)\n", buff);
        return VOS_IO_ERR;
    }
    else if (rcvSize == 0)
    {
        return VOS_NODATA_ERR;
    }
    else
    {
        *pSize = rcvSize;
        return VOS_NO_ERR;
    }
}

/**********************************************************************************************************************/
/** Bind a socket to an address and port.
 *
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      ipAddress       source IP to receive on, 0 for any
 *  @param[in]      port            port to receive on, 20548 for PD
 *
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
        vos_printf(VOS_LOG_ERROR, "bind() failed (Err: %s)\n", buff);
        return VOS_SOCK_ERR;
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Listen for incoming connections.
 *
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      backlog            maximum connection attempts if system is busy
 *
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
        vos_printf(VOS_LOG_ERROR, "listen() failed (Err: %s)\n", buff);
        return VOS_IO_ERR;
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
 *
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
                               "accept() listenFd(%d) failed (Err: %s)\n",
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
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      Input/Output error
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
        if ((errno == EINPROGRESS)
            || (errno == EWOULDBLOCK)
            || (errno == EALREADY))
        {
            return VOS_BLOCK_ERR;
        }
        else if (errno != EISCONN)
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
            vos_printf(VOS_LOG_WARNING, "connect() failed (Err: %s)\n", buff);
            return VOS_IO_ERR;
        }
    }
    return VOS_NO_ERR;
}

/**********************************************************************************************************************/
/** Send TCP data.
 *  Send data to the supplied address and port.
 *
 *  @param[in]      sock            socket descriptor
 *  @param[in]      pBuffer         pointer to data to send
 *  @param[in,out]  pSize           In: size of the data to send, Out: no of bytes sent
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be sent
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockSendTCP (
    INT32       sock,
    const UINT8 *pBuffer,
    UINT32      *pSize)
{
    ssize_t sendSize    = 0;
    size_t  bufferSize  = 0;

    if (sock == -1 || pBuffer == NULL || pSize == NULL)
    {
        return VOS_PARAM_ERR;
    }

    bufferSize  = (size_t) *pSize;
    *pSize      = 0;

    /* Keep on sending until we got rid of all data or we received an unrecoverable error */
    do
    {
        sendSize = write(sock, pBuffer, bufferSize);
        if (sendSize >= 0)
        {
            bufferSize  -= sendSize;
            pBuffer     += sendSize;
            *pSize      += sendSize;
        }
        if (sendSize == -1 && errno == EWOULDBLOCK)
        {
            return VOS_BLOCK_ERR;
        }
    }
    while (bufferSize && !(sendSize == -1 && errno != EINTR));

    if (sendSize == -1)
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_WARNING, "send() failed (Err: %s)\n", buff);
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
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   sock descriptor unknown, parameter error
 *  @retval         VOS_IO_ERR      data could not be read
 *  @retval         VOS_NODATA_ERR  no data
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
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

        if (rcvSize == -1 && errno == EWOULDBLOCK)
        {
            if (*pSize == 0)
            {
                return VOS_BLOCK_ERR;
            }
            else
            {
                return VOS_NO_ERR;
            }
        }
    }
    while ((bufferSize > 0 && rcvSize > 0) || (rcvSize == -1 && errno == EINTR));

    if ((rcvSize == -1) && !(errno == EMSGSIZE))
    {
        char buff[VOS_MAX_ERR_STR_SIZE];
        strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
        vos_printf(VOS_LOG_WARNING, "receive() failed (Err: %s)\n", buff);
        return VOS_IO_ERR;
    }
    else if (*pSize == 0)
    {
        if (errno == EMSGSIZE)
        {
            return VOS_MEM_ERR;
        }
        else
        {
            return VOS_NODATA_ERR;
        }
    }
    else
    {
        return VOS_NO_ERR;
    }
}

/**********************************************************************************************************************/
/** Set Using Multicast I/F
 *
 *  @param[in]      sock                        socket descriptor
 *  @param[in]      mcIfAddress                 using Multicast I/F Address
 *
 *  @retval         VOS_NO_ERR                  no error
 *  @retval         VOS_PARAM_ERR               sock descriptor unknown, parameter error
 *  @retval         VOS_SOCK_ERR                option not supported
 */
EXT_DECL VOS_ERR_T vos_sockSetMulticastIf (
    INT32   sock,
    UINT32  mcIfAddress)
{
    struct sockaddr_in  multicastIFAddress;
    VOS_ERR_T           result = VOS_NO_ERR;

    if (sock == -1)
    {
        result = VOS_PARAM_ERR;
    }
    else
    {
        /* Multicast I/F setting */
        memset((char *)&multicastIFAddress, 0, sizeof(multicastIFAddress));
        multicastIFAddress.sin_addr.s_addr = vos_htonl(mcIfAddress);

        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &multicastIFAddress.sin_addr, sizeof(struct in_addr)) == -1)
        {
            char buff[VOS_MAX_ERR_STR_SIZE];
            strerror_r(errno, buff, VOS_MAX_ERR_STR_SIZE);
            vos_printf(VOS_LOG_WARNING, "setsockopt() IP_MULTICAST_IF failed (Err: %s)\n", buff);
            result = VOS_SOCK_ERR;
        }
        else
        {
            /*    DEBUG
                struct sockaddr_in myAddress = {0};
                socklen_t optionSize ;
                getsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &myAddress.sin_addr, &optionSize);
            */
            result = VOS_NO_ERR;
        }
    }
    return result;
}
