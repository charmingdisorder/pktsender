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
# Try 4 (see test.out.approx4)
# just doing num_pkts vs fixed ring size
PKT_SIZE="1000"
NUM_PKTS="16 32 64 256 512 1024 2048"
PROCESS_DELAY="150"
SEND_DELAY="5"
RING_SIZE="32"
NUM_TESTS=3
# took 5m 3s
# 64(32) => 0
# 256(32) => 0.2
# 512(32) => 0.26
# 1024(32) => 0.297
# 2048(32) => 0.312
# y = f(x) is logarithmic, where x is num_packets, and y is packet loss



BATCH_WAIT=1

LOG_OUT=$BASEPATH/test.out

pktsize=0
numpkts=0
ringsize=0
send_delay=0
process_delay=0

#rm -f $LOG_OUT

function run_test()
{
        TS=`date "+%Y-%m-%d %H:%M:%S"`
        params="pkt_size = $pktsize num_pkts = $numpkts snd_delay = $send_delay rcv_delay = $process_delay rng_size = $ringsize"
        echo "$TS Started test ($params)"
        (sleep 1; $SENDER -l $pktsize -n $numpkts -w $BATCH_WAIT -i $send_delay > /dev/null; sleep 1; pkill pkt_receiver) &
        $RECEIVER -S $ringsize -d $process_delay 2>$BASEPATH/rcv_err.log 1>$BASEPATH/rcv_out.log

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
                                                 run_test
                                        done
                                done
                        done
                done
        done
done

#rm -f $BASEPATH/rcv_out.log $BASEPATH/rcv_err.log
