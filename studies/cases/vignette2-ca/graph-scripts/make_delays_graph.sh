#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BASE_NAME="$(basename $(pwd))"
CWD="$(pwd)"

for RESULT in r1 r2 r3 r4; do

rm -f delays_a2a3.plot
rm -f delays_all.plot
rm -f transport-cdf.plot
rm -f transport-cdf.plot.base
rm -f transport-cdf.eps

TYPES=('ca-0.2' 'ca-0.4' 'ca-0.6' 'ca-0.8' 'ca-coding' 'ca-frag')
NAMES=('CA-CacheCode(0.2)' 'CA-CacheCode(0.4)' 'CA-CacheCode(0.6)' 'CA-CacheCode(0.8)' 'NetCode' 'Frag')

I=1
CATS=$(ls -d */ | grep -E "^${RESULT}[0-9A-Za-z_.\-]*")
SEED=""

if [ "$CATS" = "" ]
then
   continue
fi

for CAT in $CATS; do
TYPE=$(echo $CAT | cut -d'-' -f5,6)
SEED=s$(echo $CAT | cut -d'-' -f3 | cut -d 's' -f 2)

echo "\"delays_${SEED}_${TYPE}_all.dat\" using 1:2 with linespoints ls ${I} title '${NAMES[$((I-1))]}', \\" >> delays_all.plot
cat $CAT/${TYPE}.apps_output | grep "Received," | awk -F, '{print $6;}' | sort -n | awk '{print $1, NR;}' > delays_${SEED}_${TYPE}_all.dat
I=$((I+1))
done

echo '
set term pdfcairo color
set output "transport-cdf.pdf"

# Line style for axes
set style line 80 lt rgb "#808080"

# Line style for grid
set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey

set grid back linestyle 81
set border 3 back linestyle 80 # Remove border on top and right.  These
             # borders are useless and make it harder
             # to see plotted lines near the border.
    # Also, put it in grey; no need for so much emphasis on a border.
set xtics nomirror
set ytics nomirror

# display the CDF
set autoscale y
set xrange [0:600]
set key out horiz bot center

#set title  "Delivery Performance"
set ylabel "Data objects received within delay"
set xlabel "Delay (s)"

set style line 1 lt rgb "#A00000" lw 1 pt 1 ps 1
set style line 2 lt rgb "#00A000" lw 1 pt 6 ps 1
set style line 3 lt rgb "#5060D0" lw 1 pt 2 ps 1
set style line 4 lt rgb "#F25900" lw 1 pt 9 ps 1
set style line 5 lt rgb "#4B0082" lw 1 pt 12 ps 1
set style line 6 lt rgb "#1E90FF" lw 1 pt 10 ps 1

#set style line 1 lc rgb "#0000FF" lt 1 lw 1 pt 2 ps 1
#set style line 2 lc rgb "#778899" lt 1 lw 1 pt 3 ps 1
#set style line 3 lc rgb "#FF8C00" lt 1 lw 1 pt 4 ps 1
#set style line 4 lc rgb "#9400D3" lt 1 lw 1 pt 5 ps 1
#set style line 5 lc rgb "#B22222" lt 1 lw 1 pt 6 ps 1
#set style line 6 lc rgb "#8B4513" lt 1 lw 1 pt 7 ps 1
plot \\'  > transport-cdf.plot.base
sed -i '$s/\\$//' transport-cdf.plot.base

cp transport-cdf.plot.base transport-cdf.plot
cat delays_all.plot >> transport-cdf.plot
sed -i '$s/, \\$//' transport-cdf.plot
touch transport-cdf.pdf
gnuplot transport-cdf.plot
mv transport-cdf.pdf ${BASE_NAME}-transport-cdf-${SEED}_all.pdf

done
