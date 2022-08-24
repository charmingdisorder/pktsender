#ifndef _PKT_RECEIVER_H_
#define _PKT_RECEIVER_H_
#include "proto.h"

#define PRCVR_NAME "preceiver"

/* IP address/port to listen on */
#define PRCVR_IPADDR "127.0.0.1"
#define PRCVR_PORT 31337
#define PRCVR_USE_TCP 1 /* UDP otherwise */

/* Processing options */
#define PRCVR_RING_SIZE 16  /* Ring buffer size */
#define PRCVR_LATENCY 15    /* Delay on buffer processing (in milliseconds) */

#endif
