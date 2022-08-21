/*
 * MD5 implementation based on gist (https://gist.github.com/creationix/4710780)
 *
 * License: MIT
 */

#ifndef _MY_MD5_H_
#define _MY_MD5_H_

#include <stdint.h>

struct md5_csum {
        uint32_t h0;
        uint32_t h1;
        uint32_t h2;
        uint32_t h3;
};

void md5_csum_init (size_t initial_len);
struct md5_csum md5_csum(uint8_t *initial_msg);

#endif
