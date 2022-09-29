#!/bin/bash
#
# Simple wrapper/launcher for pkt_sender suite
#
# Usage:
#
# ./run.sh [-u] [-n NUM_PKTS] [-l PKT_SIZE] [-w BATCH_WAIT] [-s RING_SIZE] [-i SEND_DELAY] [-d PROCESS_DELAY]
# (-u is UDP)


BASEPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

RECEIVER="$BASEPATH/build/pkt_receiver"
SENDER="$BASEPATH/build/pkt_sender"

use_udp=""
num_pkts=30
pkt_size=1024
process_delay=10
send_delay=10
ring_size=8
batch_wait=1

if [[ ! -f "$RECEIVER" || ! -f "$SENDER" ]]; then
        echo "Executable file doesn't exists. Build project first (see BUILD)."
        exit 1
fi


while [[ "$#" -gt 0 ]]; do
    case $1 in
            -u) use_udp="-u"; shift ;;
            -n) num_pkts="$2"; shift ;;
            -l) pkt_size="$2"; shift ;;
            -w) batch_wait="$2"; shift ;;
            -s) ring_size="$2"; shift ;;
            -i) send_delay="$2"; shift ;;
            -d) process_delay="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

TS=`date "+%Y-%m-%d %H:%M:%S"`

params="pkt_size = $pkt_size num_pkts = $num_pkts send_delay = $send_delay process_delay = $process_delay ring_size = $ring_size"

#echo "$TS Started ($params)"

(sleep 1; $SENDER $use_udp -l $pkt_size -n $num_pkts -w $batch_wait -i $send_delay > /dev/null; sleep 1; pkill pkt_receiver) &

$RECEIVER $use_udp -S $ring_size -d $process_delay 2>&1 | grep STATS |  awk '{print "processed = " $4 " dropped = " $3 " loss = " $3/($3+$4)}'

TS=`date "+%Y-%m-%d %H:%M:%S"`

#echo "$TS Finished"



