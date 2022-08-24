#ifndef _PKT_PROTO_H_
#define _PKT_PROTO_H_

#include <stdint.h>

struct pkt_header {
        /* sequence id */
        uint32_t seqid;

        /* checksum */
        uint32_t h0;
        uint32_t h1;
        uint32_t h2;
        uint32_t h3;

        /* timestamp */
        uint32_t sec;
        uint16_t msec;

        /* payload size */
        uint16_t size;
} __attribute__((packed));

#endif
