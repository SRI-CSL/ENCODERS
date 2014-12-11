#!/bin/bash

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Generate the output statistics file of the run. 
#

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$(basename ${DIR})
NOW=$(date '+%s')

OUTPUTFILE=$1
if [ ! -r "${OUTPUTFILE}" ]; then
    exit 1
fi

PRINT_STATS_HEADER=$2

bash "${DIR}/validate.sh" ${OUTPUTFILE} "/dev/null"
PR="$?"

if [ "${PR}" == "1" ]; then
    PASSED="N"
else
    PASSED="Y"
fi

NUM_NODES="%%num_nodes%%"
DURATION="%%duration%%"
HASH=$(grep ": HASH: " ${OUTPUTFILE} | awk '{print $3;}' | uniq)

TX_BYTES=$(grep "TX bytes" ${OUTPUTFILE}  | awk '{print $6;}' | sed -n 's/bytes:\(.*\)/\1/p'  | awk '{SUM+=$1;} END { print SUM };')
RX_BYTES=$(grep "RX bytes" ${OUTPUTFILE}  | awk '{print $2;}' | sed -n 's/bytes:\(.*\)/\1/p'  | awk '{SUM+=$1;} END { print SUM };')
TEMP=$(tempfile)
cat ${OUTPUTFILE} | grep "Received,"  | awk 'BEGIN { FS=","; } { if ($3 <= $4) print $6; if ($3 > $4) print $7; }' > ${TEMP}
AVG_DELAY=$(cat ${TEMP} | awk -f ${DIR}/stats.awk | awk -F, '{print $1;}')
STD_DELAY=$(cat ${TEMP} | awk -f ${DIR}/stats.awk | awk -F, '{print $2;}')
DO_DELIVERED=$(cat ${OUTPUTFILE} | grep "Received," | wc -l)
DO_PUBLISHED=$(cat ${OUTPUTFILE} | grep "Published," | wc -l)

# fine-grain statistics

HAS_STATS=$(grep Statistics ${OUTPUTFILE} | wc -l)
if [ "${HAS_STATS}" == "0" ]; then
    HAS_STATS="N"
else
    HAS_STATS="Y"
fi

STATS_DO_P_INSERTED=$(grep Statistics ${OUTPUTFILE}  | grep "\- Persistent Data Objects Inserted:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_DO_P_INSERTED}" == "" ]; then
    STATS_DO_P_INSERTED="0"
fi
STATS_DO_P_DELETED=$(grep Statistics ${OUTPUTFILE}  | grep "\- Persistent Data Objects Deleted:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_DO_P_DELETED}" == "" ]; then
    STATS_DO_P_DELETED="0"
fi
STATS_DO_INSERTED=$(grep Statistics ${OUTPUTFILE}  | grep "\- Data Objects Inserted:" | awk '{sum += $8} END {print sum}')
if [ "${STATS_DO_INSERTED}" == "" ]; then
    STATS_DO_INSERTED="0"
fi
STATS_DO_DELETED=$(grep Statistics ${OUTPUTFILE}  | grep "\- Data Objects Deleted:" | awk '{sum += $8} END {print sum}')
if [ "${STATS_DO_DELETED}" == "" ]; then
    STATS_DO_DELETED="0"
fi

# 

STATS_DO_BYTES_OUT=$(grep Statistics ${OUTPUTFILE}  | grep "\- Data Object Bytes Outgoing:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_DO_BYTES_OUT}" == "" ]; then
    STATS_DO_BYTES_OUT="0"
fi

STATS_DO_BYTES_FULLY_SENT=$(grep Statistics ${OUTPUTFILE}  | grep "\- Data Object Bytes Fully Sent:" | awk '{sum += $10} END {print sum}')
if [ "${STATS_DO_BYTES_FULLY_SENT}" == "" ]; then
    STATS_DO_BYTES_FULLY_SENT="0"
fi

STATS_DO_BYTES_INCOMING=$(grep Statistics ${OUTPUTFILE}  | grep "\- Data Object Bytes Incoming:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_DO_BYTES_INCOMING}" == "" ]; then
    STATS_DO_BYTES_INCOMING="0"
fi

STATS_DO_BYTES_FULLY_RECV=$(grep Statistics ${OUTPUTFILE}  | grep "\- Data Object Bytes Fully Received:" | awk '{sum += $10} END {print sum}')
if [ "${STATS_DO_BYTES_FULLY_RECV}" == "" ]; then
    STATS_DO_BYTES_FULLY_RECV="0"
fi

# 

STATS_DO_NOT_SENT=$(grep Statistics ${OUTPUTFILE}  | grep "\- Data Objects Not Sent:" | awk '{sum += $10} END {print sum}')
if [ "${STATS_DO_NOT_SENT}" == "" ]; then
    STATS_DO_NOT_SENT="0"
fi

STATS_DO_SENT=$(grep Statistics ${OUTPUTFILE}  | grep "\- Data Objects Fully Sent:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_DO_SENT}" == "" ]; then
    STATS_DO_SENT="0"
fi

STATS_DO_SENT_ACK=$(grep Statistics ${OUTPUTFILE}  | grep  "\- Data Objects Fully Sent and Acked:" | awk '{sum += $11} END {print sum}')
if [ "${STATS_DO_SENT_ACK}" == "" ]; then
    STATS_DO_SENT_ACK="0"
fi

STATS_DO_OUT=$(grep Statistics ${OUTPUTFILE}  | grep  "\- Data Objects Outgoing:" | awk '{sum += $8} END {print sum}')
if [ "${STATS_DO_OUT}" == "" ]; then
    STATS_DO_OUT="0"
fi

STATS_NC_DO_OUT=$(grep Statistics ${OUTPUTFILE}  | grep  "\- Non-Control Data Objects Outgoing:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_NC_DO_OUT}" == "" ]; then
    STATS_NC_DO_OUT="0"
fi

STATS_DO_INC=$(grep Statistics ${OUTPUTFILE} | grep "\- Data Objects Incoming:" | awk '{sum += $8} END {print sum}')
if [ "${STATS_DO_INC}" == "" ]; then
    STATS_DO_INC="0"
fi

STATS_NC_DO_INC=$(grep Statistics ${OUTPUTFILE} | grep "\- Non-Control Data Objects Incoming:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_NC_DO_INC}" == "" ]; then
    STATS_NC_DO_INC="0"
fi

STATS_DO_NOT_RECV=$(grep Statistics ${OUTPUTFILE} | grep "\- Data Objects Not Received:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_DO_NOT_RECV}" == "" ]; then
    STATS_DO_NOT_RECV="0"
fi

STATS_DO_FULLY_RECV=$(grep Statistics ${OUTPUTFILE} | grep "\- Data Objects Fully Received:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_DO_FULLY_RECV}" == "" ]; then
    STATS_DO_FULLY_RECV="0"
fi

#

STATS_DO_REJ=$(grep Statistics ${OUTPUTFILE} | grep "\- Data Objects Rejected:" | awk '{sum += $8} END {print sum}')
if [ "${STATS_DO_REJ}" == "" ]; then
    STATS_DO_REJ="0"
fi

STATS_DO_ACCEPT=$(grep Statistics ${OUTPUTFILE} | grep "\- Data Objects Accepted:" | awk '{sum += $8} END {print sum}')
if [ "${STATS_DO_ACCEPT}" == "" ]; then
    STATS_DO_ACCEPT="0"
fi

STATS_DO_ACK=$(grep Statistics ${OUTPUTFILE} | grep "\- Data Objects Acknowledged:" | awk '{sum += $8} END {print sum}')
if [ "${STATS_DO_ACK}" == "" ]; then
    STATS_DO_ACK="0"
fi

#

STATS_FRAG_REJ=$(grep Statistics ${OUTPUTFILE} | grep "\- Data Object Fragments Rejected:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_FRAG_REJ}" == "" ]; then
    STATS_FRAG_REJ="0"
fi

STATS_BLOCK_REJ=$(grep Statistics ${OUTPUTFILE} | grep "\- Data Object Blocks Rejected:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_BLOCK_REJ}" == "" ]; then
    STATS_BLOCK_REJ="0"
fi

#

STATS_ND_BYTES_SENT=$(grep Statistics ${OUTPUTFILE} | grep "\- Node Description Bytes Sent:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_ND_BYTES_SENT}" == "" ]; then
    STATS_ND_BYTES_SENT="0"
fi

STATS_ND_BYTES_RECV=$(grep Statistics ${OUTPUTFILE} | grep "\- Node Description Bytes Received:" | awk '{sum += $9} END {print sum}')
if [ "${STATS_ND_BYTES_RECV}" == "" ]; then
    STATS_ND_BYTES_RECV="0"
fi

STATS_ND_SENT=$(grep Statistics ${OUTPUTFILE} | grep "\- Node Descriptions Sent:" | awk '{sum += $8} END {print sum}')
if [ "${STATS_ND_SENT}" == "" ]; then
    STATS_ND_SENT="0"
fi

STATS_ND_RECV=$(grep Statistics ${OUTPUTFILE} | grep "\- Node Descriptions Received:" | awk '{sum += $8} END {print sum}')
if [ "${STATS_ND_RECV}" == "" ]; then
    STATS_ND_RECV="0"
fi

#

if [ ! -z "${PRINT_STATS_HEADER}" ]; then
    echo "name,date,passed,hash,num_nodes,duration_s,tx_bytes,rx_bytes,do_delivered,do_published,avg_delay,std_dev_delay,has_stats,do_persis_inserted,do_persis_deleted,do_inserted,do_deleted,do_bytes_outgoing,do_bytes_sent,do_bytes_incoming,do_bytes_received,do_not_sent,do_sent,do_sent_ack,do_out,non_control_do_out,do_in,non_control_do_in,do_not_recv,do_fully_recv,do_reject,do_accept,do_ack,frag_rej,block_rej,nd_bytes_sent,nd_bytes_recv,nd_sent,nd_recv"
fi

echo "${NAME},${NOW},${PASSED},${HASH},${NUM_NODES},${DURATION},${TX_BYTES},${RX_BYTES},${DO_DELIVERED},${DO_PUBLISHED},${AVG_DELAY},${STD_DELAY},${HAS_STATS},${STATS_DO_P_INSERTED},${STATS_DO_P_DELETED},${STATS_DO_INSERTED},${STATS_DO_DELETED},${STATS_DO_BYTES_OUT},${STATS_DO_BYTES_FULLY_SENT},${STATS_DO_BYTES_INCOMING},${STATS_DO_BYTES_FULLY_RECV},${STATS_DO_NOT_SENT},${STATS_DO_SENT},${STATS_DO_SENT_ACK},${STATS_DO_OUT},${STATS_NC_DO_OUT},${STATS_DO_INC},${STATS_NC_DO_INC},${STATS_DO_NOT_RECV},${STATS_DO_FULLY_RECV},${STATS_DO_REJ},${STATS_DO_ACCEPT},${STATS_DO_ACK},${STATS_FRAG_REJ},${STATS_BLOCK_REJ},${STATS_ND_BYTES_SENT},${STATS_ND_BYTES_RECV},${STATS_ND_SENT},${STATS_ND_RECV}"

rm -f ${TEMP}

exit 0
