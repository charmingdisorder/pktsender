#!/bin/sh

# Simple test suite for pkt_{sender,receiver}
#
# Tunables:
#
# Protocol (TCP/UDP)
# Number of packets
# Packet size
# Packet send interval
# Packet process delay
# Ring buffer size
# Wait time between two batches (in seconds)
#

#set -e

BASEPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
RECEIVER="$BASEPATH/build/pkt_receiver"
SENDER="$BASEPATH/build/pkt_sender"

if ! [ -x $RECEIVER ]; then
        echo "$RECEIVER doesn't exist (need to build?)"
        exit 1
fi

if ! [ -x $SENDER ]; then
        echo "$SENDER doesn't exist (need to build?)"
        exit 1
fi

# 2 * 2 * 3 * 3 * 3 = 108
#Try 1 (see test.out.approx1)
#PKT_SIZE="1000"
#NUM_PKTS="30"
#PROCESS_DELAY="10 100 1000"
#SEND_DELAY="10 100 1000"
#RING_SIZE="8 16 128"
# 108 * 3 = 324
# rng_size is the key

# Try 2 (see test.out.approx2)
#PKT_SIZE="1000"
#NUM_PKTS="40"
#PROCESS_DELAY="150"
#SEND_DELAY="10"
#RING_SIZE="8 32 128"
#NUM_TESTS=5
#
# rng_size 8->32, 0.15->0 loss

# Try 3 (see test.out.approx3)
#PKT_SIZE="1000"
#NUM_PKTS="32 64 256"
#PROCESS_DELAY="150"
#SEND_DELAY="5"
#RING_SIZE="16 32 128"
#NUM_TESTS=3
#
# num_pkts vs ring_size again
#
# Try 4 (see test.out.approx4)
# just doing num_pkts vs fixed ring size
#PKT_SIZE="1000"
#NUM_PKTS="16 32 64 256 512 1024 2048"
#PROCESS_DELAY="150"
#SEND_DELAY="5"
#RING_SIZE="32"
#NUM_TESTS=3
# took 5m 3s
# 64(32) => 0
# 256(32) => 0.2
# 512(32) => 0.26
# 1024(32) => 0.297
# 2048(32) => 0.312
# y = f(x) is logarithmic (approx), where x is num_packets, and y is packet loss
#
# Try 5 (see test.out.approx5)
# process delay vs fixed settings
#PKT_SIZE="1000"
#NUM_PKTS="512"
#PROCESS_DELAY="50 150 500 1000 1500 3000"
#SEND_DELAY="5"
#RING_SIZE="32"
#NUM_TESTS=3
# took 3.48
# constant packet loss (274)
# 2022-08-30 16:17:53 pkt_size = 1000 num_pkts = 512 snd_delay = 5 rcv_delay = 50 rng_size = 32 
# 274 750 0.267578

# Try 6 (see test.out.approx6)
# send delay vs fixed settings
#PKT_SIZE="1000"
#NUM_PKTS="256"
#PROCESS_DELAY="100"
#SEND_DELAY="5 100 300 500 1500"
#RING_SIZE="32"
#NUM_TESTS=3
# took 2m 3s
# constant packet loss (106)
#2022-08-30 17:19:38 pkt_size = 1000 num_pkts = 256 snd_delay = 5 rcv_delay = 100 rng_size = 32 
#106 406 0.207031

# Try 7 (see try 4, see test.out.approx7)
# num_pkts vs fixed ring size remix (different num pkts granularity)
#PKT_SIZE="1000"
#NUM_PKTS="500 1000 5000 10000 15000 20000"
#PROCESS_DELAY="100"
#SEND_DELAY="10"
#RING_SIZE="64"
#NUM_TESTS=3
# took 53m 47s
# TODO: NUM_TESTS should be equal to 1 for such tests
# y = f(x) is logarithmic -> 0.(3) (approx7.png)

# Try 8 (see test.out.approx8)
# remake of try #7 with pkt_size=600 (and num_tests=1)
#PKT_SIZE="600"
#NUM_PKTS="500 1000 5000 10000 15000 20000"
#PROCESS_DELAY="100"
#SEND_DELAY="10"
#RING_SIZE="64"
#NUM_TESTS=1
# took 17m 41s 
# pkt_size doesn't affect result

# Try 9
# testing different ring_size with different num_pkts with fixed pkt_size
#PKT_SIZE="600"
#NUM_PKTS="500 1000 5000"
#PROCESS_DELAY="10"
#SEND_DELAY="10"
#RING_SIZE="64 512 4096"
#NUM_TESTS=1
# took 8m 17s
#    num_pkts      500        1000       5000
# rng_size
#
#  64              202        531         3158
#  512              0          80         2707
#  4096             0          0          0

# Try 10
# testing with fixed rng_size=4096 and pkt_size=600 vs different num_pkts
#PKT_SIZE="600"
#NUM_PKTS="5000 10000 15000 20000"
#PROCESS_DELAY="1"
#SEND_DELAY="1"
#RING_SIZE="4096"
#NUM_TESTS=1
# 20:07:47 20:28:42 (20:55)
#
# num_pkts loss             time
#  5000    0                20:10:19 (2:32) 152
#  10000   2410 (12%)       20:14:45 (4:26) 266
#  15000   5694 (19%)       20:20:53 (6:08) 368
#  20000   8978 (22.5%)     20:28:42 (7:51) 491

#Try 11 (see test.out.approx11)
#PKT_SIZE="1000"
#NUM_PKTS="30"
#PROCESS_DELAY="10 100 1000"
#SEND_DELAY="10 100 1000"
#RING_SIZE="8 16 128"
# 108 * 3 = 324
# rng_size is the key

# Try 4UDP (see test.out.approx4udp)
# just doing num_pkts vs fixed ring size
#PKT_SIZE="1000"
#NUM_PKTS="16 32 64 256 512 1024 2048"
#PROCESS_DELAY="150"
#SEND_DELAY="5"
#RING_SIZE="32"
#NUM_TESTS=3
#USE_UDP="-u"

# Try 7UDP (see try 7, see test.out.approx7udp)
# num_pkts vs fixed ring size remix (different num pkts granularity)
#PKT_SIZE="1000"
#NUM_PKTS="500 1000 5000 10000 15000 20000"
#PROCESS_DELAY="100"
#SEND_DELAY="10"
#RING_SIZE="64"
#NUM_TESTS=3  # big mistake here..
#USE_UDP="-u"

# Try 10UDP
# testing with fixed rng_size=4096 and pkt_size=600 vs different num_pkts
PKT_SIZE="600"
NUM_PKTS="5000 10000 15000 20000"
PROCESS_DELAY="1"
SEND_DELAY="1"
RING_SIZE="4096"
NUM_TESTS=1
USE_UDP="-u"

BATCH_WAIT=1

LOG_OUT=$BASEPATH/test.out

pktsize=0
numpkts=0
ringsize=0
send_delay=0
process_delay=0
udp=""
#rm -f $LOG_OUT

function run_test()
{
        TS=`date "+%Y-%m-%d %H:%M:%S"`
        params="pkt_size = $pktsize num_pkts = $numpkts snd_delay = $send_delay rcv_delay = $process_delay rng_size = $ringsize"
        echo "$TS Started test ($params)"
        (sleep 1; $SENDER $USE_UDP -l $pktsize -n $numpkts -w $BATCH_WAIT -i $send_delay > /dev/null; sleep 1; pkill pkt_receiver) &
        $RECEIVER $USE_UDP -S $ringsize -d $process_delay 2>$BASEPATH/rcv_err.log 1>$BASEPATH/rcv_out.log

        if [ -n "$(grep -v PASS $BASEPATH/rcv_out.log)" ]; then
                echo "!!! Checksum fail (check $BASEPATH/rcv_out.log)"
                exit 1
        fi

        TS=`date "+%Y-%m-%d %H:%M:%S"`
        echo "$TS $params " >> $LOG_OUT
        grep STATS $BASEPATH/rcv_err.log | awk '{print $3 " " $4 " " $3/($3+$4)}' >> $LOG_OUT
#        echo >> $LOG_OUT

        #echo "$TS $params $out" >> $LOG_OUT
        echo "$TS Finished test"
}

for pktsize in $PKT_SIZE; do
        for numpkts in $NUM_PKTS; do
                for ringsize in $RING_SIZE; do
                        for send_delay in $SEND_DELAY; do
                                for process_delay in $PROCESS_DELAY; do
                                        for i in $(seq $NUM_TESTS); do
                                                udp=$USE_UDP
                                                run_test
                                        done
                                done
                        done
                done
        done
done

#rm -f $BASEPATH/rcv_out.log $BASEPATH/rcv_err.log
