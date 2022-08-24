#include <pthread.h>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "atomic_io.h"
#include "md5.h"
#include "pkt_receiver.h"
#include "pkt_sender.h"

static pthread_t listener_t;
static pthread_t processor_t;

static int verbose = 0;

static const char *ipaddr = NULL;
static uint16_t port = PRCVR_PORT;
static int is_tcp = PRCVR_USE_TCP;
static struct sockaddr_in sa;

static int sockfd = -1;

static uint16_t ring_size = PRCVR_RING_SIZE;
static uint16_t latency = PRCVR_LATENCY;

static void
usage (int ret)
{
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "\t%s [-h] [-u] [-s IPADDR] [-p PORTNUM] [-S RINGSIZE] [-l LATENCY]\n\n",
                PRCVR_NAME);

        fprintf(stderr, "\t%-16s %s\n", "-h", "Display usage information and exit");
        fprintf(stderr, "\t%-16s %s\n", "-v", "Verbose mode");

        fprintf(stderr, "\t%-16s %s\n", "-s IPADDR",
                "IP address to listen on");
        fprintf(stderr, "\t%-16s %s (%u by default)\n", "-P PORTNUM",
                "Port number to listen on", PRCVR_PORT);
        fprintf(stderr, "\t%-16s %s\n", "-u", "Use UDP protocl (TCP is used by default)");

        fprintf(stderr, "\t%-16s %s (%u by default)\n", "-S RINGSIZE", "Size of ring buffer",
                PRCVR_RING_SIZE);
        fprintf(stderr, "\t%-16s %s (%u by default)\n", "-l LATENCY",
                "Packet processing latency (in msecs)", PRCVR_LATENCY);

        exit(ret);
}

/* break connection on non-nil */
static int pkt_handle (int fd)
{
        static uint8_t buf[PSENDER_DATA_MAX_SIZE];
        struct pkt_header p;
        struct md5_csum csorig, cs;
        struct timespec ts;
        int ret;
        uint32_t seqid, sec;
        uint16_t msec, size;

        bzero(buf, PSENDER_DATA_MAX_SIZE);

        if ((ret  = atomicio(read, fd, &p, sizeof(p))) != sizeof(p)) {
                if (ret != 0)
                        fprintf(stderr, "%s: atomicio(read) returned %d (errno: %s)\n",
                                __func__, ret, strerror(errno));
                return 1;
        }

        seqid = ntohl(p.seqid);
        sec = ntohl(p.sec);
        msec = ntohs(p.msec);
        size = ntohs(p.size);

        csorig.h0 = ntohl(p.h0);
        csorig.h1 = ntohl(p.h1);
        csorig.h2 = ntohl(p.h2);
        csorig.h3 = ntohl(p.h3);

        if (size > PSENDER_DATA_MAX_SIZE - 1) {
                fprintf(stderr, "Protocol mismatch? Got size=%u, dropping..\n",
                        p.size);
                return 1;
        }

        if ((ret = atomicio(read, fd, buf, size)) != size) {
                fprintf(stderr, "%s: atomicio(read) returned %d (errno: %s)\n",
                        __func__, ret, strerror(errno));
                return 1;
        }

#ifdef DEBUG
        fprintf(stdout, "buf: %u %u\n", buf[0], buf[1]);
#endif
        cs = md5_csum(buf);

        ret = memcmp(&cs, &csorig, sizeof(cs));

#ifdef DEBUG
        printf(stdout, "orig_h0 = %lu h0 = %lu\n", csorig.h0, cs.h0);
#endif

        clock_gettime(CLOCK_MONOTONIC, &ts); /* XXX: CLOCK_REALTIME (as pkt_sender) */

        fprintf(stdout, "Received: %u %lu.%lu %s\n", seqid, ts.tv_sec, ts.tv_nsec,
                (ret == 0) ? "PASS" : "FAIL");

        return 0;
}

static void *pkt_listener_udp (void *data)
{
        return NULL;
}

static void *pkt_listener_tcp (void *data)
{
        int cfd;
        struct sockaddr_in sa;
        socklen_t len = sizeof(struct sockaddr_in);

        do {
                cfd = accept(sockfd, (struct sockaddr *)&sa, &len);

                if (cfd < 0) {
                        fprintf(stderr, "accept(): %s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                }

                while (1) {
                        if (pkt_handle(cfd))
                                break;
                }

                close(cfd);
        } while (1);

        return NULL;
}

void *pkt_processor (void *data)
{
        return NULL;
}


int
main (int argc, char **argv)
{
        int ret, opt;
        sigset_t signals;
        int signal;

        while ((opt = getopt(argc, argv, "hvus:p:n:i:w:")) != -1) {
                switch (opt) {
                case 'v':
                        verbose = 1;
                        break;
                case 'h':
                        usage(EXIT_SUCCESS);
                        break;
                case 'u':
                        is_tcp = 0;
                        break;
                case 's':
                        ipaddr = optarg;
                        break;
                case 'p':
                        {
                                int tmp = -1;
                                tmp = atoi(optarg);
                                if (tmp < 1 || tmp > 65535) {
                                        fprintf(stderr, "Incorrect port number: %s\n", optarg);
                                        exit(EINVAL);
                                }

                                port = (uint16_t) tmp;
                                break;
                        }
                case 'S':
                        {
                                int tmp = -1;
                                tmp = atoi(optarg);
                                if (tmp < 1 || tmp > 65535) {
                                        fprintf(stderr, "Incorrect ring buffer size: %s\n", optarg);
                                        exit(EINVAL);
                                }

                                port = (uint16_t) tmp;
                                break;
                        }
                case 'l':
                        {
                                int tmp = -1;
                                tmp = atoi(optarg);
                                if (tmp < 1 || tmp > 65535) {
                                        fprintf(stderr, "Incorrect latency: %s\n", optarg);
                                        exit(EINVAL);
                                }

                                latency = (uint16_t) tmp;
                                break;
                        }
                case '?':
                        fprintf(stderr, "Unknown option: %s\n", optopt);
                        exit(EINVAL);
                        break;
                }
        }

        md5_csum_init(PSENDER_DATA_MAX_SIZE);

        bzero(&sa, sizeof(sa));

        sa.sin_port = htons(port);
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = htonl(INADDR_ANY);

        if (ipaddr && (inet_pton(AF_INET, ipaddr, &sa.sin_addr)) != 1) {
                fprintf(stderr, "inet_pton() failed for '%s'\n", ipaddr);
                exit(EINVAL);
        }

        sockfd = socket(AF_INET, is_tcp ? SOCK_STREAM : SOCK_DGRAM, 0);

        if (sockfd < 0) {
                fprintf(stderr, "socket() failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
                fprintf(stderr, "bind() failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (is_tcp && (listen(sockfd, 1) != 0)) {
                fprintf(stderr, "listen() failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }


        if (pthread_create(&listener_t, NULL,
                           is_tcp ? pkt_listener_tcp : pkt_listener_udp,
                           NULL) != 0)         {
                fprintf(stderr, "pthread_create() failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (pthread_create(&processor_t, NULL, pkt_processor, NULL) != 0) {
                fprintf(stderr, "pthread_create() failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        sigemptyset(&signals);
        sigaddset(&signals, SIGINT);
        sigaddset(&signals, SIGTERM);

        while (sigwait(&signals, &signal) == 0) {
                switch (signal) {
                case SIGTERM:
                case SIGINT:
                        fprintf(stdout, "got SIGTERM, cleaning up");
                        goto out;
                default:
                        fprintf(stderr, "got signal(%d), ignoring..\n", signal);
                        break;
                }
        }

 out:
        //stop_threads();
        exit(EXIT_SUCCESS);
}


