#ifndef _PKT_SENDER_H_
#define _PKT_SENDER_H_
#include "proto.h"

#include <stdint.h>

#define PSENDER_NAME "psender"

/* Destination specification: hostname, port, protocol */
#define PSENDER_IPADDR "127.0.0.1"
#define PSENDER_PORT 31337
#define PSENDER_USE_TCP 1 /* UDP otherwise */

/* Payload buffer specification */
#define PSENDER_DATA_MIN_SIZE 600
#define PSENDER_DATA_MAX_SIZE 1600
#define PSENDER_DATA_SIZE PSENDER_DATA_MIN_SIZE  /* Use minimum size by default */

typedef uint16_t word_t;  /* word is defined as int16 */

/* Sending options */
#define PSENDER_NUM_PKTS 1000   /* Number of packets to send */
#define PSENDER_INTERVAL 10     /* Interval between packets sending (in milliseconds) */
#define PSENDER_WAIT_TIME 10    /* Interval between batch sending (in seconds) */

#endif
