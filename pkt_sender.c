#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
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
#include "pkt_sender.h"

static int sockfd = -1;
static int urandomfd = -1;

static int verbose = 0;

static int is_tcp = PSENDER_USE_TCP;

static const char *ipaddr = PSENDER_IPADDR;
static uint16_t port = PSENDER_PORT;
static struct sockaddr_in sa;

static uint16_t bufsize = PSENDER_DATA_SIZE;
static unsigned int numpkts = PSENDER_NUM_PKTS;
static unsigned int wait_time = PSENDER_WAIT_TIME;
static unsigned long interval = PSENDER_INTERVAL;

/* XXX: just to supress -Wincompatible-pointer-types for atomicio */
ssize_t my_write (int fd, void *buf, size_t count) {
        return write(fd, (const void *)buf, count);
}

static void
usage (int ret)
{
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "\t%s  [-h] [-v] [-u] [-s IPADDR] [-p PORTNUM] [-l BUFLEN] [-n PKTNUM] [-i MSECS] [-w SECS]\n\n",
                PSENDER_NAME);

        fprintf(stderr, "\t%-16s %s\n", "-h", "Display usage information and exit");
        fprintf(stderr, "\t%-16s %s\n", "-v", "Verbose mode");

        fprintf(stderr, "\t%-16s %s (\"%s\" by default)\n", "-s IPADDR",
                "Destination IPv4 address", PSENDER_IPADDR);
        fprintf(stderr, "\t%-16s %s (%u by default)\n", "-P PORTNUM",
                "Destination port number", PSENDER_PORT);
        fprintf(stderr, "\t%-16s %s\n", "-u", "Use UDP protocl (TCP is used by default)");

        fprintf(stderr, "\t%-16s %s (%u by default, the minimum is %u, the maximum is %u)\n", "-l BUFLEN",
                "Payload buffer size", PSENDER_DATA_SIZE, PSENDER_DATA_MIN_SIZE, PSENDER_DATA_MAX_SIZE);

        fprintf(stderr, "\t%-16s %s (%u by default)\n", "-n PKTNUM",
                "Number of packets to send", PSENDER_NUM_PKTS);

        fprintf(stderr, "\t%-16s %s (%u by default)\n", "-i MSECS",
                "Interval between packets transmissions (in msecs)", PSENDER_INTERVAL);

        fprintf(stderr, "\t%-16s %s (%u by default)\n", "-w SECS",
                "Interval between batch sending (in secs)", PSENDER_WAIT_TIME);

        exit(ret);
}

static inline void
fill_buffer(uint8_t *buf, size_t bufsize)
{
        if (atomicio(read, urandomfd, buf, bufsize) < 0) {
                fprintf(stderr, "read() /dev/urandom failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
}

static void
send_pkt()
{
        struct md5_csum cs;
        static uint32_t seqid = 0;
        struct timespec ts;
        struct pkt_header p;
        uint8_t payload_buf[PSENDER_DATA_MAX_SIZE];

        bzero(payload_buf, PSENDER_DATA_MAX_SIZE);

        fill_buffer(payload_buf, bufsize);

        clock_gettime(CLOCK_MONOTONIC, &ts); /* XXX: is CLOCK_REALTIME needed? */

        cs = md5_csum(payload_buf);

        p.seqid = htonl(seqid);

        p.h0 = htonl(cs.h0);
        p.h1 = htonl(cs.h1);
        p.h2 = htonl(cs.h2);
        p.h3 = htonl(cs.h3);

        p.sec = htonl(ts.tv_sec);
        p.msec = htons((ts.tv_nsec + 1.0e6/2)/1.0e6);

        p.size = htons(bufsize);

#if 0
        uint16_t tmp = (ts.tv_nsec + 1.0e6/2)/1.0e6;
        fprintf(stdout, "ts.tv_nsec = %lu p.msec = %u bufsize = %u\n",ts.tv_nsec, tmp, bufsize);

#endif

#ifdef DEBUG
        fprintf(stdout, "buf: %u %u\n", payload_buf[0], payload_buf[1]);
#endif

        if (atomicio(my_write, sockfd, &p, sizeof(p)) <= 0 ||
            atomicio(my_write, sockfd, payload_buf, bufsize) <= 0) {
                fprintf(stderr, "write() failed: %s\n", strerror(errno));
                close(sockfd);
                exit(EXIT_FAILURE);
        }

        fprintf(stdout, "Sent: %u %lu.%lu\n", seqid, ts.tv_sec, ts.tv_nsec);

        seqid++;

}

static void
send_pkts()
{
        unsigned int i;

        for (i = 0; i < numpkts; i++) {
                usleep(interval * 1000);
                send_pkt();
        }

        sleep(wait_time);

        for (i = 0; i < numpkts; i++) {
                usleep(interval * 1000);
                send_pkt();
        }
}

int
main (int argc, char **argv)
{
        int opt;

        while ((opt = getopt(argc, argv, "hvus:p:l:n:i:w:")) != -1) {
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
                case 'l':
                        {
                                int tmp = -1;
                                tmp = atoi(optarg);

                                if (tmp < PSENDER_DATA_MIN_SIZE || tmp > PSENDER_DATA_MAX_SIZE) {
                                        fprintf(stderr, "Incorrect buffer size: %s\n", optarg);
                                        exit(EINVAL);
                                }

                                bufsize = (uint16_t) tmp;
                                break;
                        }
                case 'w':
                        {
                                int tmp = -1;
                                tmp = atoi(optarg);

                                if (tmp < 1) {
                                        fprintf(stderr, "Incorrect wait time: %s\n", optarg);
                                        exit(EINVAL);
                                }

                                wait_time = tmp;
                                break;
                        }
                case 'n':
                        {
                                int tmp = -1;
                                tmp = atoi(optarg);

                                if (tmp < 1) {
                                        fprintf(stderr, "Incorrect packets number: %s\n", optarg);
                                        exit(EINVAL);
                                }

                                numpkts = tmp;
                                break;
                        }
                case '?':
                        fprintf(stderr, "Unknown option: %c\n", optopt);
                        exit(EINVAL);
                        break;
                }
        }

        urandomfd = open("/dev/urandom", O_RDONLY);

        if (urandomfd < 0) {
                fprintf(stderr, "open() for '/dev/urandom' failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        md5_csum_init(PSENDER_DATA_MAX_SIZE);

        bzero(&sa, sizeof(struct sockaddr_in));

        if (inet_pton(AF_INET, ipaddr, &sa.sin_addr) != 1) {
                fprintf(stderr, "inet_pton() failed for '%s'\n", ipaddr);
                exit(EINVAL);
        }

        sa.sin_port = htons(port);
        sa.sin_family = AF_INET;

        sockfd = socket(AF_INET, is_tcp ? SOCK_STREAM : SOCK_DGRAM, 0);

        if (sockfd < 0) {
                fprintf(stderr, "socket() failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        /* try to avoid fragmentation (XXX: think twice) */
        opt = IP_PMTUDISC_DO;
        if (setsockopt(sockfd, IPPROTO_IP, IP_MTU_DISCOVER, (const void *) &opt, sizeof(opt)) < 0) {
                fprintf(stderr, "setsockopt() failed: %s\n", strerror(errno));
                /* exit(EXIT_FAILURE); */
        }

        if (connect(sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0) {
                fprintf(stderr, "connect() failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (verbose)
                printf("Connection established, sending packets..\n");

        send_pkts();

        close(sockfd);
        close(urandomfd);

        if (verbose)
                printf("Done\n");

        return 0;
}
