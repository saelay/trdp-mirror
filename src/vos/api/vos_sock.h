/**********************************************************************************************************************/
/**
 * @file            vos_sock.h
 *
 * @brief           Typedefs for OS abstraction
 *
 * @details         This is the declaration for the OS independend socket interface
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          Bernd Loehr, NewTec GmbH
 *
 * @remarks All rights reserved. Reproduction, modification, use or disclosure
 *          to third parties without express authority is forbidden,
 *          Copyright Bombardier Transportation GmbH, Germany, 2012.
 *
 *
 * $Id$
 *
 */

#ifndef VOS_SOCK_H
#define VOS_SOCK_H

/***********************************************************************************************************************
 * INCLUDES
 */

#include "vos_types.h"

#ifdef WIN32
#include <winsock2.h>
#elif defined(VXWORKS)
#else
#include <sys/select.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * DEFINES
 */

/** The maximum number of sockets influences memory usage; for small systems we should define a smaller set */

#if MD_SUPPORT                      /* ### Eventually this could be changed to be target dependant... */

#ifndef VOS_MAX_SOCKET_CNT
#define VOS_MAX_SOCKET_CNT  80      /**< The maximum number of concurrent usable sockets per application session */
#endif
#ifndef VOS_MAX_MULTICAST_CNT
#define VOS_MAX_MULTICAST_CNT  20   /**< The maximum number of multicast groups one socket can join              */
#endif

#else

#define VOS_MAX_SOCKET_CNT      4   /**< The maximum number of concurrent usable sockets per application session */
#ifndef VOS_MAX_MULTICAST_CNT
#define VOS_MAX_MULTICAST_CNT   5   /**< The maximum number of multicast groups one socket can join              */
#endif

#endif

#define VOS_TTL_MULTICAST       64  /**< The maximum number of hops a multicast packet can take    */
#ifndef VOS_MAX_IF_NAME_SIZE
#define VOS_MAX_IF_NAME_SIZE    16  /**< The maximum size for the interface name                   */
#endif
#ifndef VOS_MAX_NUM_IF
#define VOS_MAX_NUM_IF  4           /**< The maximum number of IP interface adapters that can be handled by VOS */
#endif
#ifndef VOS_MAX_NUM_UNICAST
#define VOS_MAX_NUM_UNICAST  10     /**< The maximum number of unicast addresses that can be handled by VOS    */
#endif
#ifndef VOS_MAC_SIZE
#define VOS_MAC_SIZE  6             /**< The MAC size supported by VOS */
#endif

/***********************************************************************************************************************
 * TYPEDEFS
 */

/** Common socket options  */
typedef struct
{
    UINT8   qos;            /**< quality/type of service 0...7                     */
    UINT8   ttl;            /**< time to live for unicast (default 64)             */
    UINT8   ttl_multicast;  /**< time to live for multicast                        */
    BOOL    reuseAddrPort;  /**< allow reuse of address and port                   */
    BOOL    nonBlocking;    /**< use non blocking calls                            */
} VOS_SOCK_OPT_T;

typedef fd_set VOS_FDS_T;

typedef struct
{
    CHAR8           name[VOS_MAX_IF_NAME_SIZE]; /**< interface adapter name        */
    VOS_IP4_ADDR_T  ipAddr;                     /**< IP address                    */
    VOS_IP4_ADDR_T  netMask;                    /**< subnet mask                   */
    UINT8           mac[VOS_MAC_SIZE];          /**< interface adapter MAC address */
} VOS_IF_REC_T;

/***********************************************************************************************************************
 * PROTOTYPES
 */

/**********************************************************************************************************************/
/** Byte swapping 2 Bytes.
 *
 *  @param[in]          val             Initial value.
 *
 *  @retval             swapped value
 */

EXT_DECL UINT16 vos_htons (
    UINT16 val);

/**********************************************************************************************************************/
/** Byte swapping 2 Bytes.
 *
 *  @param[in]          val             Initial value.
 *
 *  @retval             swapped value
 */

EXT_DECL UINT16 vos_ntohs (
    UINT16 val);

/**********************************************************************************************************************/
/** Byte swapping 4 Bytes.
 *
 *  @param[in]          val             Initial value.
 *
 *  @retval             swapped value
 */

EXT_DECL UINT32 vos_htonl (
    UINT32 val);

/**********************************************************************************************************************/
/** Byte swapping 4 Bytes.
 *
 *  @param[in]          val             Initial value.
 *
 *  @retval             swapped value
 */

EXT_DECL UINT32 vos_ntohl (
    UINT32 val);

/**********************************************************************************************************************/
/** Convert IP address from dotted dec. to !host! endianess
 *
 *  @param[in]          pDottedIP     IP address as dotted decimal.
 *
 *  @retval             address in UINT32 in host endianess
 */

EXT_DECL UINT32 vos_dottedIP (
    const CHAR8 *pDottedIP);

/**********************************************************************************************************************/
/** Convert IP address to dotted dec. from !host! endianess
 *
 *  @param[in]          ipAddress   address in UINT32 in host endianess
 *
 *  @retval             IP address as dotted decimal.
 */

EXT_DECL const CHAR8 *vos_ipDotted (
    UINT32 ipAddress);

/**********************************************************************************************************************/
/** Check if the supplied address is a multicast group address.
 *
 *  @param[in]          ipAddress      IP address to check.
 *
 *  @retval             TRUE           address is a multicast address
 *  @retval             FALSE          address is not a multicast address
 */

EXT_DECL BOOL vos_isMulticast (
    UINT32 ipAddress);

/**********************************************************************************************************************/
/** Get a list of interface addresses
 *  The caller has to provide an array of interface records to be filled.
 *
 *  @param[in,out]  pAddrCnt          in:   pointer to array size of interface record
 *                                    out:  pointer to number of interface records read
 *  @param[in,out]  ifAddrs           array of interface records
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pAddrCnt and/or ifAddrs == NULL
 *  @retval         VOS_MEM_ERR     memory allocation error
 *  @retval         VOS_SOCK_ERR    GetAdaptersInfo() error
 */
EXT_DECL VOS_ERR_T vos_getInterfaces(
    UINT32         * pAddrCnt,
    VOS_IF_REC_T ifAddrs[]);

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
    VOS_TIME_T  *pTimeOut);

/*    Sockets    */

/**********************************************************************************************************************/
/** Initialize the socket library.
 *  Must be called once before any other call
 *
 *  @retval         VOS_NO_ERR            no error
 *  @retval         VOS_SOCK_ERR          sockets not supported
 */

EXT_DECL VOS_ERR_T vos_sockInit (
    void);

/**********************************************************************************************************************/
/** Return the MAC address of the default adapter.
 *
 *  @param[out]     pMAC            return MAC address.
 *
 *  @retval         VOS_NO_ERR      no error
 *  @retval         VOS_PARAM_ERR   pMAC == NULL
 *  @retval         VOS_SOCK_ERR    socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockGetMAC(
    UINT8 pMAC[VOS_MAC_SIZE]);

/**********************************************************************************************************************/
/** Create an UDP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *    applied later.
 *    Note: Some target systems might not support every option.
 *
 *  @param[out]     pSock             pointer to socket descriptor returned
 *  @param[in]      pOptions          pointer to socket options (optional)
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_PARAM_ERR     pSock == NULL
 *  @retval         VOS_SOCK_ERR      socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockOpenUDP (
    INT32                   *pSock,
    const VOS_SOCK_OPT_T    *pOptions);

/**********************************************************************************************************************/
/** Create a TCP socket.
 *  Return a socket descriptor for further calls. The socket options are optional and can be
 *    applied later.
 *
 *  @param[out]     pSock             pointer to socket descriptor returned
 *  @param[in]      pOptions          pointer to socket options (optional)
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_PARAM_ERR     pSock == NULL
 *  @retval         VOS_SOCK_ERR      socket not available or option not supported
 */

EXT_DECL VOS_ERR_T vos_sockOpenTCP (
    INT32                   *pSock,
    const VOS_SOCK_OPT_T    *pOptions);

/**********************************************************************************************************************/
/** Close a socket.
 *  Release any resources aquired by this socket
 *
 *  @param[in]      sock              socket descriptor
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_PARAM_ERR     pSock == NULL
 */

EXT_DECL VOS_ERR_T vos_sockClose (
    INT32 sock);

/**********************************************************************************************************************/
/** Set socket options.
 *  Note: Some target systems might not support each option.
 *
 *  @param[in]      sock              socket descriptor
 *  @param[in]      pOptions          pointer to socket options (optional)
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_PARAM_ERR     parameter out of range/invalid
 */

EXT_DECL VOS_ERR_T vos_sockSetOptions (
    INT32                   sock,
    const VOS_SOCK_OPT_T    *pOptions);

/**********************************************************************************************************************/
/** Join a multicast group.
 *  Note: Some target systems might not support this option.
 *
 *  @param[in]      sock              socket descriptor
 *  @param[in]      mcAddress         multicast group to join
 *  @param[in]      ipAddress         depicts interface on which to join, default 0 for any
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_PARAM_ERR     parameter out of range/invalid
 *  @retval         VOS_SOCK_ERR      option not supported
 */

EXT_DECL VOS_ERR_T vos_sockJoinMC (
    INT32   sock,
    UINT32  mcAddress,
    UINT32  ipAddress);

/**********************************************************************************************************************/
/** Leave a multicast group.
 *  Note: Some target systems might not support this option.
 *
 *  @param[in]      sock              socket descriptor
 *  @param[in]      mcAddress         multicast group to join
 *  @param[in]      ipAddress         depicts interface on which to leave, default 0 for any
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_INIT_ERR      module not initialised
 *  @retval         VOS_NOINIT_ERR    invalid handle
 *  @retval         VOS_PARAM_ERR     parameter out of range/invalid
 *  @retval         VOS_SOCK_ERR      option not supported
 */

EXT_DECL VOS_ERR_T vos_sockLeaveMC (
    INT32   sock,
    UINT32  mcAddress,
    UINT32  ipAddress);

/**********************************************************************************************************************/
/** Send UDP data.
 *  Send data to the given address and port.
 *
 *  @param[in]      sock               socket descriptor
 *  @param[in]      pBuffer            pointer to data to send
 *  @param[in,out]  pSize              In: size of the data to send, Out: no of bytes sent
 *  @param[in]      ipAddress          destination IP
 *  @param[in]      port               destination port
 *
 *  @retval         VOS_NO_ERR         no error
 *  @retval         VOS_PARAM_ERR      parameter out of range/invalid
 *  @retval         VOS_IO_ERR         data could not be sent
 *  @retval         VOS_BLOCK_ERR   Call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockSendUDP (
    INT32       sock,
    const UINT8 *pBuffer,
    UINT32      *pSize,
    UINT32      ipAddress,
    UINT16      port);

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
 *  @param[in]      peek            if true, leave data in queue
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
    UINT32  *pDstIPAddr,
    BOOL    peek);

/**********************************************************************************************************************/
/** Bind a socket to an address and port.
 *
 *
 *  @param[in]      sock              socket descriptor
 *  @param[in]      ipAddress         source IP to receive from, 0 for any
 *  @param[in]      port              port to receive from
 *
 *  @retval         VOS_NO_ERR        no error
 *  @retval         VOS_PARAM_ERR     parameter out of range/invalid
 *  @retval         VOS_IO_ERR        Input/Output error
 *  @retval         VOS_MEM_ERR       resource error
 */

EXT_DECL VOS_ERR_T vos_sockBind (
    INT32   sock,
    UINT32  ipAddress,
    UINT16  port);

/**********************************************************************************************************************/
/** Listen for incoming TCP connections.
 *
 *
 *  @param[in]      sock               socket descriptor
 *  @param[in]      backlog            maximum connection attempts if system is busy
 *
 *  @retval         VOS_NO_ERR         no error
 *  @retval         VOS_PARAM_ERR      parameter out of range/invalid
 *  @retval         VOS_IO_ERR         Input/Output error
 *  @retval         VOS_MEM_ERR        resource error
 */

EXT_DECL VOS_ERR_T vos_sockListen (
    INT32   sock,
    UINT32  backlog);

/**********************************************************************************************************************/
/** Accept an incoming TCP connection.
 *  Accept incoming connections on the provided socket. May block and will return a new socket descriptor when
 *    accepting a connection. The original socket *pSock, remains open.
 *
 *  @param[in]      sock               Socket descriptor
 *  @param[out]     pSock              Pointer to socket descriptor, on exit new socket
 *  @param[out]     pIPAddress         source IP to receive on, 0 for any
 *  @param[out]     pPort              port to receive on, 20548 for PD
 *
 *  @retval         VOS_NO_ERR         no error
 *  @retval         VOS_PARAM_ERR      NULL parameter, parameter error
 *  @retval         VOS_UNKNOWN_ERR    sock descriptor unknown error
 */

EXT_DECL VOS_ERR_T vos_sockAccept (
    INT32   sock,
    INT32   *pSock,
    UINT32  *pIPAddress,
    UINT16  *pPort);

/**********************************************************************************************************************/
/** Open a TCP connection.
 *
 *
 *  @param[in]      sock               socket descriptor
 *  @param[in]      ipAddress          destination IP
 *  @param[in]      port               destination port
 *
 *  @retval         VOS_NO_ERR         no error
 *  @retval         VOS_PARAM_ERR      parameter out of range/invalid
 *  @retval         VOS_IO_ERR         Input/Output error
 */

EXT_DECL VOS_ERR_T vos_sockConnect (
    INT32   sock,
    UINT32  ipAddress,
    UINT16  port);

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
 *  @retval         VOS_BLOCK_ERR   call would have blocked in blocking mode, data partially sent
 */

EXT_DECL VOS_ERR_T vos_sockSendTCP (
    INT32       sock,
    const UINT8 *pBuffer,
    UINT32      *pSize);

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
 *  @retval         VOS_NODATA_ERR  no data in non-blocking
 *  @retval         VOS_BLOCK_ERR   call would have blocked in blocking mode
 */

EXT_DECL VOS_ERR_T vos_sockReceiveTCP (
    INT32   sock,
    UINT8   *pBuffer,
    UINT32  *pSize);

/**********************************************************************************************************************/
/** Set Using Multicast I/F
 *
 *  @param[in]      sock                       socket descriptor
 *  @param[in]      mcIfAddress                using Multicast I/F Address
 *
 *  @retval         VOS_NO_ERR                 no error
 *  @retval         VOS_PARAM_ERR              sock descriptor unknown, parameter error
 */
EXT_DECL VOS_ERR_T vos_sockSetMulticastIf (
    INT32   sock,
    UINT32  mcIfAddress);


#ifdef __cplusplus
}
#endif

#endif /* VOS_SOCK_H */
