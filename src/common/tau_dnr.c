/**********************************************************************************************************************/
/**
 * @file            tau_dnr.c
 *
 * @brief           Functions for domain name resolution
 *
 * @details
 *
 * @note            Project: TCNOpen TRDP prototype stack
 *
 * @author          B. Loehr (initial version)
 *
 * @remarks This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 *          If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *          Copyright Bombardier Transportation Inc. or its subsidiaries and others, 2013. All rights reserved.
 *
 * $Id$
 *
 *      BL 2015-12-14: Ticket #8: DNR client
 */

/***********************************************************************************************************************
 * INCLUDES
 */

#include <string.h>
#include <stdio.h>

#include "tau_dnr.h"
#include "trdp_utils.h"
#include "vos_sock.h"


/***********************************************************************************************************************
 * DEFINES
 */

#define TAU_MAX_HOSTS_LINE_LENGTH   120
#define TAU_MAX_NO_CACHE_ENTRY      50
#define TAU_MAX_NO_IF               4       /**< Default interface should be in the first 4 */
#define TAU_MAX_DNS_BUFFER_SIZE     1500    /* if this doesn't suffice, we need to allocate it */
#define TAU_MAX_URI_SIZE            64      /* host part only, without user part */
#define TAU_MAX_NAME_SIZE           256     /* Allocated on stack */
#define TAU_DNS_TIME_OUT_LONG       10      /**< Timeout in seconds for DNS server reply, if no hosts file provided   */
#define TAU_DNS_TIME_OUT_SHORT      1       /**< Timeout in seconds for DNS server reply, if hosts file was provided  */


/***********************************************************************************************************************
 * TYPEDEFS
 */

typedef struct tau_dnr_cache
{
    CHAR8           uri[TRDP_MAX_URI_HOST_LEN];
    TRDP_IP_ADDR_T  ipAddr;
    UINT32          topoCnt;
} TAU_DNR_ENTRY_T;

typedef struct tau_dnr_data
{
    TRDP_IP_ADDR_T  ecspIpAddr;
    UINT16          ecspPort;
    UINT16          timeout;
    UINT32          noOfCachedEntries;
    TAU_DNR_ENTRY_T cache[TAU_MAX_NO_CACHE_ENTRY];
} TAU_DNR_DATA_T;

typedef struct tau_dnr_query
{
    UINT16  id;
    UINT16  param;
    UINT16  noOfQueries;
    UINT16  noOfReplies;
    UINT16  noOfAuth;
    UINT16  noOfAddInfo;
    UINT8   data[];
} TAU_DNR_QUERY_T;

/* Constant sized fields of the resource record structure */
#pragma pack(push, 1)
typedef struct R_DATA
{
    UINT16      type;
    UINT16      class;
    UINT32      ttl;
    UINT16      data_len;
} GNU_PACKED TAU_R_DATA_T;

/* DNS header structure */
typedef struct DNS_HEADER
{
    UINT16          id;         /* identification number */
    UINT8           param1;     /* Bits 7...0   */
    UINT8           param2;     /* Bits 15...8  */
    UINT16          q_count;    /* number of question entries */
    UINT16          ans_count;  /* number of answer entries */
    UINT16          auth_count; /* number of authority entries */
    UINT16          add_count;  /* number of resource entries */
}  GNU_PACKED TAU_DNS_HEADER_T;
#pragma pack(pop)

/* Pointers to resource record contents */
typedef struct RES_RECORD
{
    unsigned char   *name;
    TAU_R_DATA_T    *resource;
    unsigned char   *rdata;
} TAU_RES_RECORD_T;


/* Constant sized fields of query structure */
typedef struct QUESTION
{
    UINT16  qtype;
    UINT16  qclass;
} TAU_DNR_QUEST_T;

/***********************************************************************************************************************
 *   Locals
 */

//TAU_DNR_DATA_T gDNR = {0x0a000001, 53, 0};  /**< default DNR/ECSP settings  */

#pragma mark ----------------------- Local -----------------------------

/***********************************************************************************************************************
 *   Local functions
 */

/**********************************************************************************************************************/
/**    Function to read and convert a resource from the DNS response
 *
 *  @param[in]      pReader         Pointer to current read position
 *  @param[in]      pBuffer         Pointer to start of packet buffer
 *  @param[out]     pCount          Pointer to return skip in read position
 *  @param[in]      pNameBuffer     Pointer to a 256 Byte buffer to copy the result to
 *
 *  @retval         pointer to returned resource string
 */
static CHAR8 *readName (UINT8 *pReader, UINT8 *pBuffer, UINT32 *pCount, CHAR8 *pNameBuffer)
{
    CHAR8           *pName = pNameBuffer;
    unsigned int    p = 0, jumped = 0, offset;
    int             i, j;

    *pCount  = 1;
    pName[0] = '\0';

    /* read the names in 3www6newtec2de0 format */
    while (*pReader != 0 && p < TAU_MAX_NAME_SIZE)
    {
        if (*pReader >= 192)
        {
            offset  = (*pReader) * 256 + *(pReader + 1) - 49152;        /* 49152 = 11000000 00000000 ;) */
            pReader  = pBuffer + offset - 1;
            jumped  = 1;                                /* we have jumped to another location so counting wont go up! */
        }
        else
        {
            pName[p++] = *pReader;
        }

        pReader = pReader + 1;

        if (jumped == 0)
        {
            *pCount = *pCount + 1;                  /* if we havent jumped to another location then we can count up */
        }
    }

    pName[p] = '\0';                                 /* string complete */

    if (jumped == 1)
    {
        *pCount = *pCount + 1;                      /* number of steps we actually moved forward in the packet */
    }

    /* now convert 3www6newtec2de0 to www.newtec.de */

    for (i = 0; i < (int)strlen((const char *)pName) && i < TAU_MAX_NAME_SIZE; i++)
    {
        p = pName[i];
        for (j = 0; j < (int)p; j++)
        {
            pName[i] = pName[i + 1];
            i       = i + 1;
        }
        pName[i] = '.';
    }

    pName[i - 1] = '\0'; /* remove the last dot */

    return pName;
}

/**********************************************************************************************************************/
/** Function to convert www.newtec.de to 3www6newtec2de0
 *
 *  @param[in]      pDns            Pointer to destination position
 *  @param[in]      pHost           Pointer to source string
 *
 *  @retval         none
 */
static void changetoDnsNameFormat (UINT8 *pDns, CHAR8 *pHost)
{
    int lock = 0, i;

    strcat((char *)pHost, ".");

    for (i = 0; i < (int)strlen((char *)pHost); i++)
    {
        if (pHost[i] == '.')
        {
            *pDns++ = i - lock;
            for (; lock < i; lock++)
            {
                *pDns++ = pHost[lock];
            }
            lock++;
        }
    }
    *pDns++ = '\0';
}

static int compareURI ( const void *arg1, const void *arg2 )
{
    return vos_strnicmp(arg1, arg2, TRDP_MAX_URI_HOST_LEN);
}

static void printDNRcache (TAU_DNR_DATA_T *pDNR)
{
    UINT32 i;
    for (i = 0; i < pDNR->noOfCachedEntries; i++)
    {
        vos_printLog(VOS_LOG_DBG, "%03d:\t%0u.%0u.%0u.%0u\t%s\t(topo: 0x%08x)\n", i,
                     pDNR->cache[i].ipAddr >> 24,
                     (pDNR->cache[i].ipAddr >> 16) & 0xFF,
                     (pDNR->cache[i].ipAddr >> 8) & 0xFF,
                     pDNR->cache[i].ipAddr & 0xFF,
                     pDNR->cache[i].uri,
                     pDNR->cache[i].topoCnt);
    }
}

/**********************************************************************************************************************/
/**    Function to populate the cache from a hosts file
 *
 *  @param[in]      pDNR                Pointer to dnr data
 *  @param[in]      pHostsFileName      Hosts file name as ECSP replacement
 *
 *  @retval         none
 */
static void readHostsFile (
    TAU_DNR_DATA_T      *pDNR,
    const CHAR8         *pHostsFileName)
{
    FILE    *fp = fopen(pHostsFileName, "r");
    CHAR8   line[TAU_MAX_HOSTS_LINE_LENGTH];

    if (fp != NULL)
    {
        /* while not end of file */
        while (!feof(fp) && pDNR->noOfCachedEntries < TAU_MAX_NO_CACHE_ENTRY)
        {
            /* get a line from the file */
            if (fgets(line, TAU_MAX_HOSTS_LINE_LENGTH, fp) != NULL)
            {
                unsigned int    ip1, ip2, ip3, ip4;
                int             noOfItems = 0;
                /* Try to get IP and URI */
                noOfItems = sscanf(line,
                                   "%d.%d.%d.%d %64s",
                                   &ip1,
                                   &ip2,
                                   &ip3,
                                   &ip4,
                                   pDNR->cache[pDNR->noOfCachedEntries].uri);
                if (noOfItems == 5)
                {
                    pDNR->cache[pDNR->noOfCachedEntries].uri[TRDP_MAX_URI_HOST_LEN - 1] = 0;
                    pDNR->cache[pDNR->noOfCachedEntries].ipAddr   = ip1 << 24 | ip2 << 16 | ip3 << 8 | ip4;
                    pDNR->cache[pDNR->noOfCachedEntries].topoCnt  = 0;
                    pDNR->noOfCachedEntries++;
                }
            }
        }
        vos_printLog(VOS_LOG_DBG, "readHostsFile: %d entries processed\n", pDNR->noOfCachedEntries);
        vos_qsort(pDNR->cache, pDNR->noOfCachedEntries, sizeof(TAU_DNR_ENTRY_T), compareURI);
        fclose(fp);
    }
    else
    {
        vos_printLogStr(VOS_LOG_ERROR, "readHostsFile: Not found!\n");
    }
    printDNRcache(pDNR);
}

/**********************************************************************************************************************/
/**    create and send a DNS query
 *
 *  @param[in]      sock            Socket descriptor
 *  @param[in]      pUri            Pointer to host name
 *  @param[in]      id              ID to identify reply
 *  @param[in]      pSize           Last entry which was invalid, can be NULL
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
static TRDP_ERR_T createSendQuery (
    TAU_DNR_DATA_T      *pDNR,
    INT32               sock,
    const CHAR8         *pUri,
    UINT16              id,
    UINT32              *pSize)
{
    CHAR8               strBuf[TAU_MAX_URI_SIZE + 3];      /* conversion enlarges this buffer */
    UINT8               packetBuffer[TAU_MAX_DNS_BUFFER_SIZE], *pBuf;
    TAU_DNS_HEADER_T    *pHeader = (TAU_DNS_HEADER_T *) packetBuffer;
    UINT32              size;
    TRDP_ERR_T          err;

    if (pUri == NULL || strlen(pUri) == 0 || pSize == NULL)
    {
        vos_printLogStr(VOS_LOG_ERROR, "createSendQuery has no search string\n");
        return TRDP_PARAM_ERR;
    }

    memset(packetBuffer, 0, TAU_MAX_DNS_BUFFER_SIZE);

    pHeader->id         = vos_htons(id);
    pHeader->param1     = 0x1;              /* Recursion desired */
    pHeader->param2     = 0x0;              /* all zero */
    pHeader->q_count    = vos_htons(1);

    pBuf = (UINT8 *) (pHeader + 1);

    strncpy((char *)strBuf, pUri, TAU_MAX_URI_SIZE);
    changetoDnsNameFormat(pBuf, strBuf);

    *pSize = (UINT32) strlen((char *)strBuf) + 1;

    pBuf    += *pSize;
    *pBuf++ = 0;                /* Query type 'A'   */
    *pBuf++ = 1;
    *pBuf++ = 0;                /* Query class ?   */
    *pBuf++ = 1;
    size    = (UINT32) (pBuf - packetBuffer);
    *pSize  += 4;               /* add query type and class size! */

    /* send the query */
    err = vos_sockSendUDP(sock, packetBuffer, &size, pDNR->ecspIpAddr, pDNR->ecspPort);
    if (err != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "createSendQuery failed to sent a query!\n");
        return err;
    }
    return err;
}

/**********************************************************************************************************************/
static void parseResponse (
    UINT8           *pPacket,
    UINT32          size,
    UINT16          id,
    UINT32          querySize,
    TRDP_IP_ADDR_T  *pIP_addr)
{
    TAU_DNS_HEADER_T    *dns = (TAU_DNS_HEADER_T *) pPacket;
    UINT8 *pReader;
    UINT32 i, skip;
    CHAR8   name[256];
    TAU_RES_RECORD_T   answers[20] /*, auth[20], addit[20]*/; /* the replies from the DNS server */

    /* move ahead of the dns header and the query field */
    pReader = pPacket + sizeof(TAU_DNS_HEADER_T) + querySize;

    vos_printLog(VOS_LOG_DBG, "The response contains : \n");
    vos_printLog(VOS_LOG_DBG, " %d Questions.\n", vos_ntohs(dns->q_count));
    vos_printLog(VOS_LOG_DBG, " %d Answers.\n", vos_ntohs(dns->ans_count));
    vos_printLog(VOS_LOG_DBG, " %d Authoritative Servers.\n", vos_ntohs(dns->auth_count));
    vos_printLog(VOS_LOG_DBG, " %d Additional records.\n", vos_ntohs(dns->add_count));

    /* reading answers */

    for (i = 0; i < vos_ntohs(dns->ans_count); i++)
    {
        readName(pReader, pPacket, &skip, name);
        pReader += skip;

        answers[i].resource = (TAU_R_DATA_T *)(pReader);
        pReader = pReader + sizeof(TAU_R_DATA_T);

        if (vos_ntohs(answers[i].resource->type) == 1) /* if its an ipv4 address */
        {
            if (vos_ntohs(answers[i].resource->data_len) != 4)
            {
                vos_printLog(VOS_LOG_ERROR,
                             "*** DNS server promised IPv4 address, but returned %d Bytes!!!\n",
                             vos_ntohs(answers[i].resource->data_len));
                *pIP_addr = VOS_INADDR_ANY;
            }

            *pIP_addr = pReader[0] << 24 | pReader[1] << 16 | pReader[2] << 8 | pReader[3];
            vos_printLog(VOS_LOG_INFO, "%s -> 0x%08x\n", name, *pIP_addr);

            pReader = pReader + vos_ntohs(answers[i].resource->data_len);

        }
        else
        {
            answers[i].rdata = (UINT8 *) readName(pReader, pPacket, &skip, name);
            pReader += skip;
        }

    }
#if 0   /* Will we ever need more? Just in case... */

    /* read authorities */
    for (i = 0; i < vos_ntohs(dns->auth_count); i++)
    {
        auth[i].name    = (UINT8*) readName(pReader, pPacket, &skip, name);
        pReader         += skip;

        auth[i].resource = (struct R_DATA *)(pReader);
        pReader += sizeof(struct R_DATA);

        auth[i].rdata   = (UINT8*) readName(pReader, pPacket, &skip, name);
        pReader         += skip;
    }

    /* read additional */
    for (i = 0; i < vos_ntohs(dns->add_count); i++)
    {
        addit[i].name   = (UINT8*) readName(pReader, pPacket, &skip, name);
        pReader         += skip;

        addit[i].resource = (struct R_DATA *)(pReader);
        pReader += sizeof(struct R_DATA);

        if (vos_ntohs(addit[i].resource->type) == 1)
        {
            int j;
            addit[i].rdata = (UINT8*)vos_memAlloc(vos_ntohs(addit[i].resource->data_len));
            for (j = 0; j < vos_ntohs(addit[i].resource->data_len); j++)
            {
                addit[i].rdata[j] = pReader[j];
            }

            addit[i].rdata[vos_ntohs(addit[i].resource->data_len)] = '\0';
            pReader += vos_ntohs(addit[i].resource->data_len);

        }
        else
        {
            addit[i].rdata  = (UINT8*) readName(pReader, pPacket, &skip, name);
            pReader         += skip;
        }
    }

    /* print answers */
    for (i = 0; i < vos_ntohs(dns->ans_count); i++)
    {
        /* printf("\nAnswer : %d",i+1); */
        vos_printLog(VOS_LOG_DBG, "Name : %s ", answers[i].name);

        if (vos_ntohs(answers[i].resource->type) == 1) /* IPv4 address */
        {
            vos_printLog(VOS_LOG_DBG, "has IPv4 address : %s", vos_ipDotted(*(UINT32 *)answers[i].rdata));
        }
        if (vos_ntohs(answers[i].resource->type) == 5) /* Canonical name for an alias */
        {
            vos_printLog(VOS_LOG_DBG, "has alias name : %s", answers[i].rdata);
        }

        printf("\n");
    }

    /* print authorities */
    for (i = 0; i < vos_ntohs(dns->auth_count); i++)
    {
        /* printf("\nAuthorities : %d",i+1); */
        vos_printLog(VOS_LOG_DBG, "Name : %s ", auth[i].name);
        if (vos_ntohs(auth[i].resource->type) == 2)
        {
            vos_printLog(VOS_LOG_DBG, "has authoritative nameserver : %s", auth[i].rdata);
        }
        vos_printLog(VOS_LOG_DBG, "\n");
    }

    /* print additional resource records */
    for (i = 0; i < vos_ntohs(dns->add_count); i++)
    {
        /* printf("\nAdditional : %d",i+1); */
        vos_printLog(VOS_LOG_DBG, "Name : %s ", addit[i].name);
        if (vos_ntohs(addit[i].resource->type) == 1)
        {
            vos_printLog(VOS_LOG_DBG, "has IPv4 address : %s", vos_ipDotted(*(UINT32 *)addit[i].rdata));
        }
        vos_printLog(VOS_LOG_DBG, "\n");
    }
#endif

}

/**********************************************************************************************************************/
/**    Query the DNS server for the addresses
 *
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *  @param[in]      pTemp               Last entry which was invalid, can be NULL
 *  @param[in]      pUri                Pointer to host name
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
static void updateDNSentry (
    TRDP_APP_SESSION_T  appHandle,
    TAU_DNR_ENTRY_T     *pTemp,
    const CHAR8         *pUri)
{
    INT32           my_socket;
    VOS_ERR_T       err;
    UINT8           packetBuffer[TAU_MAX_DNS_BUFFER_SIZE];
    UINT32          size, querySize;
    VOS_SOCK_OPT_T  opts;
    UINT16          id      = (UINT16) appHandle;
    TAU_DNR_DATA_T  *pDNR = (TAU_DNR_DATA_T *) appHandle->pUser;
    TRDP_IP_ADDR_T  ip_addr = VOS_INADDR_ANY;

    memset(&opts, 0, sizeof(opts));

    err = vos_sockOpenUDP(&my_socket, &opts);

    if (err != VOS_NO_ERR)
    {
        vos_printLogStr(VOS_LOG_ERROR, "updateDNSentry failed to open socket\n");
        return;
    }

    err = createSendQuery(pDNR, my_socket, pUri, id, &querySize);

    if (err != VOS_NO_ERR)
    {
        goto exit;
    }

    /* wait for reply */
    while (1)
    {
        VOS_FDS_T   rfds;
        TRDP_TIME_T tv = {pDNR->timeout, 0};
        int         rv;

        FD_ZERO(&rfds);
        FD_SET(my_socket, &rfds);

        rv = vos_select(my_socket + 1, &rfds, NULL, NULL, &tv);

        if (rv > 0 && FD_ISSET(my_socket, &rfds))
        {
            /* Clear our packet buffer  */
            memset(packetBuffer, 0, TAU_MAX_DNS_BUFFER_SIZE);
            size = TAU_MAX_DNS_BUFFER_SIZE;
            /* Get what was announced */
            vos_sockReceiveUDP(my_socket, packetBuffer, &size, &pDNR->ecspIpAddr, &pDNR->ecspPort, NULL, FALSE);
            FD_CLR(my_socket, &rfds);
            
            if (size == 0)
            {
                continue;       /* Try again, if there was no data */
            }
            /*  Get and convert response */
            parseResponse(packetBuffer, size, id, querySize, &ip_addr);

            if (pTemp != NULL && ip_addr != VOS_INADDR_ANY)
            {
                /* Overwrite outdated entry */
                pTemp->ipAddr   = ip_addr;
                pTemp->topoCnt  = appHandle->etbTopoCnt;
                break;
            }
            else /* It's a new one, update our cache */
            {
                int cacheEntry = pDNR->noOfCachedEntries;

                if (cacheEntry >= TAU_MAX_NO_CACHE_ENTRY)   /* Cache is full! */
                {
                    cacheEntry = 0;                         /* Overwrite first */
                }
                else
                {
                    pDNR->noOfCachedEntries++;
                }

                /* Position found, store everything */
                strncpy(pDNR->cache[cacheEntry].uri, pUri, TAU_MAX_URI_SIZE);
                pDNR->cache[cacheEntry].ipAddr   = ip_addr;
                pDNR->cache[cacheEntry].topoCnt  = appHandle->etbTopoCnt;

                /* Sort the entries to get faster hits  */
                vos_qsort(pDNR->cache, pDNR->noOfCachedEntries, sizeof(TAU_DNR_ENTRY_T), compareURI);
                break;
            }
        }
        else
        {
            break;
        }
    }

exit:
    vos_sockClose(my_socket);
    return;
}

#pragma mark ----------------------- Public -----------------------------

/***********************************************************************************************************************
 *   Public
 */

/**********************************************************************************************************************/
/**    Function to init DNR
 *
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *  @param[in]      ecspIpAddr          IP address of DNR server (default 10.0.0.1)
 *  @param[in]      ecspPort            Port of DNR server (default 53)
 *  @param[in]      pHostsFileName       Optional hosts file name as ECSP replacement
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *  @retval         TRDP_INIT_ERR   initialisation error
 *
 */
EXT_DECL TRDP_ERR_T tau_initDnr (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_IP_ADDR_T      ecspIpAddr,
    UINT16              ecspPort,
    const CHAR8         *pHostsFileName)
{
    TAU_DNR_DATA_T *pDNR;  /**< default DNR/ECSP settings  */

    if (appHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    pDNR = (TAU_DNR_DATA_T*) vos_memAlloc(sizeof(TAU_DNR_DATA_T));
    if (pDNR == NULL)
    {
        return TRDP_MEM_ERR;
    }
    
    /* save to application session */
    appHandle->pUser  = pDNR;

    pDNR->ecspIpAddr = (ecspIpAddr == 0)? 0x0a000001 : ecspIpAddr;
    pDNR->ecspPort = (ecspPort == 0)? 53 : ecspPort;
    
    /* Get locally defined hosts */
    if (pHostsFileName != NULL && strlen(pHostsFileName) > 0)
    {
        readHostsFile(pDNR, pHostsFileName);
        pDNR->timeout = TAU_DNS_TIME_OUT_SHORT;
    }
    else
    {
        pDNR->noOfCachedEntries = 0;
        pDNR->timeout = TAU_DNS_TIME_OUT_LONG;
    }
    return TRDP_NO_ERR;
}

/**********************************************************************************************************************/
/**    Function to deinit DNR
 *
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL void tau_deInitDnr (
    TRDP_APP_SESSION_T  appHandle)
{
    
    if (appHandle != NULL && appHandle->pUser != NULL)
    {
        vos_memFree(appHandle->pUser);
        appHandle->pUser = NULL;
    }
}

/**********************************************************************************************************************/
/**    Who am I ?.
 *  Realizes a kind of 'Who am I' function. It is used to determine the own identifiers (i.e. the own labels),
 *  which may be used as host part of the own fully qualified domain name.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession()
 *  @param[out]     devId           Returns the device label (host name)
 *  @param[out]     vehId           Returns the vehicle label
 *  @param[out]     cstId           Returns the consist label
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_getOwnIds (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_LABEL_T        devId,
    TRDP_LABEL_T        vehId,
    TRDP_LABEL_T        cstId)
{
    return TRDP_NO_ERR;
}

/* ------------------------------------------------------------------------------------------------------------------ */

/**********************************************************************************************************************/
/**    Function to get the own IP address.
 *  Returns the IP address set by openSession. If it was 0 (INADDR_ANY), the address of the default adapter will be
 *  returned.
 *
 *  @param[in]      appHandle           Handle returned by tlc_openSession()
 *
 *  @retval         own IP address
 *
 */
EXT_DECL TRDP_IP_ADDR_T tau_getOwnAddr (
    TRDP_APP_SESSION_T appHandle)
{
    if (appHandle != NULL)
    {
        if (appHandle->realIP == VOS_INADDR_ANY)     /* Default interface? */
        {
            UINT32          i;
            UINT32          addrCnt = VOS_MAX_NUM_IF;
            VOS_IF_REC_T    localIF[VOS_MAX_NUM_IF];
            vos_getInterfaces(&addrCnt, localIF);
            for (i = 0; i < addrCnt; ++i)
            {
                if (localIF[i].mac[0] ||        /* Take a MAC address as indicator for an ethernet interface */
                    localIF[i].mac[1] ||
                    localIF[i].mac[2] ||
                    localIF[i].mac[3] ||
                    localIF[i].mac[4] ||
                    localIF[i].mac[5])
                {
                    return localIF[i].ipAddr;   /* Could still be unset!    */
                }
            }
        }
    }


    return VOS_INADDR_ANY;
}



/**********************************************************************************************************************/
/**    Function to convert a URI to an IP address.
 *
 *  Receives an URI as input variable and translates this URI to an IP-Address.
 *  The URI may specify either a unicast or a multicast IP-Address.
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession()
 *  @param[out]     pAddr           Pointer to return the IP address
 *  @param[in]      pUri            Pointer to an URI or an IP Address string, NULL==own URI
 *
 *  @retval         TRDP_NO_ERR         no error
 *  @retval         TRDP_PARAM_ERR      Parameter error
 *  @retval         TRDP_UNRESOLVED_ERR Could not resolve error
 *  @retval         TRDP_TOPO_ERR       Cache/DB entry is invalid
 *
 */
EXT_DECL TRDP_ERR_T tau_uri2Addr (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_IP_ADDR_T      *pAddr,
    const TRDP_URI_T    pUri)
{
    TAU_DNR_DATA_T  *pDNR;
    TAU_DNR_ENTRY_T *pTemp;
    int i;

    if (appHandle == NULL ||
        pAddr == NULL)
    {
        return TRDP_PARAM_ERR;
    }

    /* If no URI given, we return our own address   */
    if (pUri == NULL)
    {
        *pAddr = tau_getOwnAddr(appHandle);
        return TRDP_NO_ERR;
    }
    
    pDNR = (TAU_DNR_DATA_T*) appHandle->pUser;

    /* Check for dotted IP address  */
    if ((*pAddr = vos_dottedIP(pUri)) != VOS_INADDR_ANY)
    {
        return TRDP_NO_ERR;
    }
    /* Look inside the cache    */
    for (i = 0; i < 2; ++i)
    {
        pTemp = (TAU_DNR_ENTRY_T *) vos_bsearch(pUri, pDNR->cache, pDNR->noOfCachedEntries, sizeof(TAU_DNR_ENTRY_T),
                                                compareURI);
        if (pTemp != NULL &&
            (pTemp->topoCnt == appHandle->etbTopoCnt ||     /* Do the topocounts match? */
             appHandle->etbTopoCnt == 0))                   /* Or do we not care?       */
        {
            *pAddr = pTemp->ipAddr;
            return TRDP_NO_ERR;
        }
        else    /* address is not known or out of date (topocounts differ)  */
        {
            updateDNSentry(appHandle, pTemp, pUri);
            /* try resolving again... */
        }
    }

    *pAddr = VOS_INADDR_ANY;
    return TRDP_UNRESOLVED_ERR;
}



/**********************************************************************************************************************/
/**    Function to convert an IP address to a URI.
 *  Receives an IP-Address and translates it into the host part of the corresponding URI.
 *  Both unicast and multicast addresses are accepted.
 *
 *
 *  @param[in]      appHandle       Handle returned by tlc_openSession()
 *  @param[out]     pUri            Pointer to a string to return the URI host part
 *  @param[in]      addr            IP address, 0==own address
 *
 *  @retval         TRDP_NO_ERR     no error
 *  @retval         TRDP_PARAM_ERR  Parameter error
 *
 */
EXT_DECL TRDP_ERR_T tau_addr2Uri (
    TRDP_APP_SESSION_T  appHandle,
    TRDP_URI_HOST_T     pUri,
    TRDP_IP_ADDR_T      addr)
{
    TAU_DNR_DATA_T  *pDNR;
    if (appHandle == NULL)
    {
        return TRDP_PARAM_ERR;
    }
    
    pDNR = (TAU_DNR_DATA_T*) appHandle->pUser;

    if (addr == 0)
    {
    }
    else
    {
        int i;
        for (i = 0; i < pDNR->noOfCachedEntries; ++i)
        {
            if (pDNR->cache[i].ipAddr == addr &&
                (appHandle->etbTopoCnt == 0 || (pDNR->cache[i].topoCnt == appHandle->etbTopoCnt)))
            {
                strncpy(pUri, pDNR->cache[i].uri, TRDP_MAX_URI_HOST_LEN);
                return TRDP_NO_ERR;
            }
        }
        /* address not in cache: Make reverse request */
        
    }
    return TRDP_NO_ERR;
}

/* ---------------------------------------------------------------------------- */
