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

#include "md5.h"
#include "pkt_sender.h"

static int sockfd = -1;
static int urandomfd = -1;

static int verbose = 0;

static int is_tcp = 1;

static const char *ipaddr = PSENDER_IPADDR;
static uint16_t port = PSENDER_PORT;
static struct sockaddr_in sa;

static uint8_t *payload_buf = NULL;
static uint16_t bufsize = PSENDER_DATA_SIZE;
static unsigned int numpkts = PSENDER_NUM_PKTS;
static unsigned int wait_time = PSENDER_WAIT_TIME;
static unsigned long interval = PSENDER_INTERVAL;

static ssize_t
writen(int fd, const void *vptr, size_t n)
{
        size_t nleft;
        ssize_t nwritten;
        const char*ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
                        if (errno == EINTR)
                                nwritten = 0;/* and call write() again */
                        else
                                return(-1);/* error */
                }

                nleft -= nwritten;
                ptr   += nwritten;
        }
        return(n);
}

static size_t
readn (int fd, unsigned char *ptr, int nbytes)
{
        int nleft, nread;

        nleft = nbytes;

        while (nleft > 0)
        {
                nread = read(fd, ptr, nleft);

                if (nread <= 0)
                        return (nread); /* error */

                nleft -= nread;
                ptr   += nread;
        }

        return (nbytes - nleft);
}

static void
usage (int ret)
{
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "\t%s  [-h] [-u] [-s IPADDR] [-p PORTNUM] [-l BUFLEN] [-n PKTNUM] [-i MSECS] [-w SECS]\n\n",
                PSENDER_NAME);

        fprintf(stderr, "\t%-16s %s\n", "-h", "Display usage information and exit");

        fprintf(stderr, "\t%-16s %s (\"%s\" by default)\n", "-s HOSTNAME",
                "Destination IPv4 address", PSENDER_IPADDR);
        fprintf(stderr, "\t%-16s %s (%u by default)\n", "-P PORTNUM",
                "Destination port number", PSENDER_PORT);

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
fill_buffer()
{
        if (read(urandomfd, payload_buf, (size_t)bufsize) < 0) {
                fprintf(stderr, "read() /dev/urandom failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }
}

static void
send_pkt ()
{
        struct md5_csum cs;
        uint8_t *iter;
        static uint32_t pktnum = 0;
        ssize_t r;
        struct timespec ts;

        if (readn(urandomfd, payload_buf, (size_t)bufsize) <= 0) {
                fprintf(stderr, "read() /dev/urandom failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        clock_gettime(CLOCK_MONOTONIC, &ts);

        iter = payload_buf;
        cs = md5_csum(payload_buf);

        *iter = htonl(pktnum); iter+=4;

        *iter = htonl(cs.h0); iter+=4;
        *iter = htonl(cs.h1); iter+=4;
        *iter = htonl(cs.h2); iter+=4;
        *iter = htonl(cs.h3); iter+=4;

        *iter = htonl(ts.tv_sec); iter+=4;
        *iter = htonl(ts.tv_nsec); iter+=4;

        memcpy(iter, payload_buf, bufsize);

        r = writen(sockfd, payload_buf, 4+16+8+bufsize);

        if (r < 0) {
                fprintf(stderr, "write() failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        fprintf(stdout, "Sent: %u %lu.%lu\n", pktnum, ts.tv_sec, ts.tv_nsec);

        pktnum++;
}

static void
send_pkts()
{
        unsigned int i;

        for (i = 0; i < numpkts; i++) {
                send_pkt();
                usleep(interval * 1000);
        }

        sleep(wait_time);

        for (i = 0; i < numpkts; i++) {
                send_pkt();
                usleep(interval * 1000);
        }

}

int
main (int argc, char **argv)
{
        int opt, ret;

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
                }

        }

        urandomfd = open("/dev/urandom", O_RDONLY);

        if (urandomfd < 0) {
                fprintf(stderr, "open() for '/dev/urandom' failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        md5_csum_init(bufsize);

        payload_buf = malloc(4 + 16 + 8 + bufsize);   /* num(2,uint16) + csum(4 + 4 + 4 + 4) +  */
        if (payload_buf == NULL) {
                fprintf(stderr, "malloc() failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        bzero(&sa, sizeof(struct sockaddr_in));

        ret = inet_pton(AF_INET, ipaddr, &sa.sin_addr);

        if (ret != 1) {
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

        /* try to avoid fragmentation */
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
