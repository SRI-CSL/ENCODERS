#!/bin/bash
PRINT_HEADER=$2
NAME=$(echo $1 | awk 'BEGIN { FS="-"; } { print $7; }' | sed 's/\(.*\)\.sh_logs.*/\1/')
N=$(echo $1 | awk 'BEGIN { FS="-"; } { print $3; }' | sed 's/\(.*\)N/\1/')
K=$(echo $1 | awk 'BEGIN { FS="-"; } { print $5; }' | sed 's/\(.*\)K/\1/')
D=$(echo $1 | awk 'BEGIN { FS="-"; } { print $4; }' | sed 's/\(.*\)B.*/\1/')
RX=$(grep "RX bytes" $1  | awk '{print $2;}' | sed -n 's/bytes:\(.*\)/\1/p'  | awk '{SUM+=$1;} END { print SUM };')
TX=$(grep "TX bytes" $1  | awk '{print $6;}' | sed -n 's/bytes:\(.*\)/\1/p'  | awk '{SUM+=$1;} END { print SUM };')

AVG_DELAY=$(cat $1 | grep "Received"  | awk 'BEGIN { FS=","; } { if ($3 <= $4) print $6; if ($3 > $4) print $7; }' | awk '{ SUM += $1; C+=1 } END { printf "%2f", SUM/C }')

NUM_DO_RECEIVED=$(cat $1 | grep "Received" | wc -l)

PNAME=""

if [ ${NAME} == "NO_FORWARD" ]; then
    PNAME="none"
elif [ ${NAME} == "FLOOD" ]; then
    PNAME="EPIDEMIC"
else
    PNAME=${NAME}
fi

if [ ${PRINT_HEADER} ]; then
    echo "N=$N,K=$K,D=$D"
    printf "%-18s %-16s %-18s %-16s %-16s\n" "# node name" "rx" "tx" "delay" "DO recv."
fi
printf "%-12s %16d %16d %16f %16d\n" ${PNAME} ${RX} ${TX} ${AVG_DELAY} ${NUM_DO_RECEIVED}
#echo "Total RX: ${RX} TX: ${TX}, Delay: ${AVG_DELAY}, num recv: ${NUM_DO_RECEIVED}"

#DIRECT       2692093228      2694383765      2.687457        1623
