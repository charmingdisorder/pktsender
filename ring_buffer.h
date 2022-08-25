/*
 * ring_buffer.h - poor man's implementation of ring buffer somewhat based on
 * https://github.com/AndersKaloer/Ring-Buffer/
 *
 * Copyright (c) 2022, Alexey Mikhailov
 * Copyright (c) 2014, Anders Kalør
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <pthread.h>
#include <stdint.h>

#include "pkt_sender.h"
#include "proto.h"

#define RING_BUFFER_SIZE 16
#define RING_BUFFER_MASK (RING_BUFFER_SIZE-1)

int is_terminating = 0;

struct ring_element_t {
        struct pkt_header h;
        uint8_t buf[PSENDER_DATA_MAX_SIZE];
};

struct ring_buffer_t {
        struct ring_element_t buffer[RING_BUFFER_SIZE];
        uint8_t tail_index;
        uint8_t head_index;
        uint32_t received;
        uint32_t dropped;
        uint32_t processed;

        pthread_mutex_t mtx;
        pthread_cond_t empty;
};

static inline void ring_buffer_print_stats(struct ring_buffer_t *ring)
{
        fprintf(stderr, "STATS: received = %lu, dropped = %lu, processed = %lu\n",
                ring->received, ring->dropped, ring->processed);
}

static void ring_buffer_init(struct ring_buffer_t *ring) {
        ring->tail_index = 0;
        ring->head_index = 0;
        ring->received = 0;
        ring->processed = 0;

        for (int i = 0; i < RING_BUFFER_SIZE; i++) {
                bzero(ring->buffer[i].buf, sizeof(struct ring_element_t));
        }

        if (pthread_mutex_init(&ring->mtx, NULL) != 0) {
                fprintf(stderr, "pthread_mutex_init()\n");
                exit(EXIT_FAILURE);
        }

        if (pthread_cond_init(&ring->empty, NULL) != 0) {
                fprintf(stderr, "pthread_cond_init()\n");
                exit(EXIT_FAILURE);
        }
}

static inline uint8_t is_ring_buffer_empty(struct ring_buffer_t *ring) {
        return (ring->head_index == ring->tail_index);
}

static inline uint8_t is_ring_buffer_full(struct ring_buffer_t *ring) {
        return ((ring->head_index - ring->tail_index) & RING_BUFFER_MASK) == RING_BUFFER_MASK;
}

static int
ring_buffer_queue(struct ring_buffer_t *ring, struct pkt_header *p, uint8_t *buf)
{
        pthread_mutex_lock(&ring->mtx);

        ring->received++;

        if(is_ring_buffer_full(ring)) {
                ring->dropped++;
                pthread_mutex_unlock(&ring->mtx);
        }

        memcpy(&ring->buffer[ring->head_index].h, &p, sizeof(p));
        memcpy(ring->buffer[ring->head_index].buf, buf, p->size);

        ring->head_index = ((ring->head_index + 1) & RING_BUFFER_MASK);
        pthread_cond_broadcast(&ring->empty);
        pthread_mutex_unlock(&ring->mtx);

        return 0;
}

static int
ring_buffer_dequeue(struct ring_buffer_t *ring, struct pkt_header *p, uint8_t *buf)
{
        struct timespec ts;
        int ret;

        pthread_mutex_lock(&ring->mtx);

        if (is_ring_buffer_empty(ring)) {
                while (1) {
                        ts.tv_sec = time(NULL) + 3;
                        ts.tv_nsec = 0;
                        ret = pthread_cond_timedwait(&ring->empty, &ring->mtx, &ts);

                        if (ret == ETIMEDOUT && is_terminating) {
                                return 1;
                        }
                }
        }

        memcpy(p, &ring->buffer[ring->tail_index].h, sizeof(*p));
        memcpy(buf, &ring->buffer[ring->tail_index].buf, p->size);

        ring->tail_index = (ring->tail_index+1) & RING_BUFFER_MASK;

        pthread_mutex_unlock(&ring->mtx);

        return 0;
}

static void ring_buffer_init(struct ring_buffer_t *buffer);
static int ring_buffer_queue(struct ring_buffer_t *ring, struct pkt_header *p, uint8_t *buf);
static int ring_buffer_dequeue(struct ring_buffer_t *ring, struct pkt_header *p, uint8_t *buf);

#endif
