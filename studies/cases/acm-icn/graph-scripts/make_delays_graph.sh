#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BASE_NAME="$(basename $(pwd))"
CWD="$(pwd)"

for RESULT in r1 r2 r3; do
rm -f delays_a2a3.plot
rm -f delays_all.plot
rm -f transport-cdf.plot
rm -f transport-cdf.plot.base
rm -f transport-cdf.eps

I=1
CATS=$(ls -d */ | grep -E "^$RESULT")
SEED=""

for CAT in $CATS; do
TYPE=$(echo $CAT | cut -d'-' -f4,5,6 | grep -oEi "(encryption)|(encryption-static)|(no-security)|(signatures)|(signatures-static)")
SEED=s$(echo $CAT | cut -d'-' -f2 | cut -d 's' -f 2)

echo "\"delays_${SEED}_${TYPE}_all.dat\" using 1:2 with linespoints ls ${I} title '${TYPE}', \\" >> delays_all.plot
cat $CAT/$TYPE.apps_output | grep "Received," | awk -F, '{print $6;}' | sort -n | awk '{print $1, NR;}' > delays_${SEED}_${TYPE}_all.dat
I=$((I+1))
done

echo '
set term post eps color
set output "transport-cdf.eps"

# display the CDF
set autoscale y
set xrange [0:900]
set key right bottom
set title  "System Performance"
set ylabel "Data objects received by time t"
set xlabel "Time (s)"
set style line 1 lc rgb "#0000FF" lt 1 lw 2 pt 2 pi 10 ps 0.75
set style line 2 lc rgb "#228B22" lt 1 lw 2 pt 7 pi 10 ps 0.75
set style line 3 lc rgb "#FF8C00" lt 1 lw 2 pt 4 pi 10 ps 0.75
set style line 4 lc rgb "#9400D3" lt 1 lw 2 pt 5 pi 10 ps 0.75
set style line 5 lc rgb "#B22222" lt 1 lw 2 pt 6 pi 10 ps 0.75
plot \\'  > transport-cdf.plot.base
sed -i '$s/\\$//' transport-cdf.plot.base

cp transport-cdf.plot.base transport-cdf.plot
cat delays_all.plot >> transport-cdf.plot
sed -i '$s/, \\$//' transport-cdf.plot
touch transport-cdf.eps
gnuplot transport-cdf.plot
mv transport-cdf.eps ${BASE_NAME}-transport-cdf-${SEED}_all.eps

done
