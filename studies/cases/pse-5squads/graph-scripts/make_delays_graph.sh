#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BASE_NAME="$(basename $(pwd))"
CWD="$(pwd)"

for RESULT in r1 r2 r3 r4; do
for SECURITY in security-off security-dynamic security-static; do

rm -f delays_a2a3.plot
rm -f delays_all.plot
rm -f transport-cdf.plot
rm -f transport-cdf.plot.base
rm -f transport-cdf.eps

I=1
CATS=$(ls -d */ | grep -E "^${RESULT}[0-9A-Za-z_.\-]*${SECURITY}[0-9A-Za-z_.\-]*")
SEED=""

if [ "$CATS" = "" ]
then
   continue
fi

for CAT in $CATS; do
TYPE=$(echo $CAT | cut -d'-' -f5,6,7 | grep -oEi "(NONE|PRIORITY|PRIORITY-UDISS|PRIORITY-UDISS-UCACHING|PRIORITY-UDISS-SCACHING)")_$(echo $CAT | cut -d'-' -f10,11,12 | grep -oEi "(UDISS1|UDISS2)")
SEED=s$(echo $CAT | cut -d'-' -f3 | cut -d 's' -f 2)

echo "\"delays_${SECURITY}_${SEED}_${TYPE}_a2a3.dat\" using 1:2 with linespoints ls ${I} title '${TYPE}', \\" >> delays_a2a3.plot
cat $CAT/${TYPE}_${SECURITY}.apps_output | grep "Received," | egrep -v "1024,\(null\)" | egrep -v "5120,\(null\)" | egrep -v "10240,\(null\)" | awk -F, '{print $6;}' | sort -n | awk '{print $1, NR;}' > delays_${SECURITY}_${SEED}_${TYPE}_a2a3.dat
echo "\"delays_${SECURITY}_${SEED}_${TYPE}_all.dat\" using 1:2 with linespoints ls ${I} title '${TYPE}', \\" >> delays_all.plot
cat $CAT/${TYPE}_${SECURITY}.apps_output | grep "Received," | awk -F, '{print $6;}' | sort -n | awk '{print $1, NR;}' > delays_${SECURITY}_${SEED}_${TYPE}_all.dat
I=$((I+1))
done

echo '
set term post eps color
set output "transport-cdf.eps"

# display the CDF
set autoscale y
set xrange [0:1800]
set key right bottom
set title  "Data objects delivered within delay"
set ylabel "Number of data objects within delay"
set xlabel "Delay (s)"
set style line 1 lc rgb "#0000FF" lt 1 lw 1 pt 2 pi -1 ps 1
set style line 2 lc rgb "#778899" lt 1 lw 1 pt 3 pi -1 ps 1
set style line 3 lc rgb "#FF8C00" lt 1 lw 1 pt 4 pi -1 ps 1
set style line 4 lc rgb "#9400D3" lt 1 lw 1 pt 5 pi -1 ps 1
set style line 5 lc rgb "#B22222" lt 1 lw 1 pt 6 pi -1 ps 1
set style line 6 lc rgb "#8B4513" lt 1 lw 1 pt 7 pi -1 ps 1
plot \\'  > transport-cdf.plot.base
sed -i '$s/\\$//' transport-cdf.plot.base

cp transport-cdf.plot.base transport-cdf.plot
cat delays_a2a3.plot >> transport-cdf.plot
sed -i '$s/, \\$//' transport-cdf.plot
touch transport-cdf.eps
gnuplot transport-cdf.plot
mv transport-cdf.eps ${BASE_NAME}-transport-cdf-${SECURITY}-${SEED}_a2a3.eps

echo '
set term post eps color
set output "transport-cdf.eps"

# display the CDF
set autoscale y
set xrange [0:1800]
set key right bottom
set title  "Data objects delivered within delay"
set ylabel "Number of data objects within delay"
set xlabel "Delay (s)"
set style line 1 lc rgb "#0000FF" lt 1 lw 1 pt 2 pi 20 ps 1
set style line 2 lc rgb "#778899" lt 1 lw 1 pt 3 pi 20 ps 1
set style line 3 lc rgb "#FF8C00" lt 1 lw 1 pt 4 pi 20 ps 1
set style line 4 lc rgb "#9400D3" lt 1 lw 1 pt 5 pi 20 ps 1
set style line 5 lc rgb "#B22222" lt 1 lw 1 pt 6 pi 20 ps 1
set style line 6 lc rgb "#8B4513" lt 1 lw 1 pt 7 pi 20 ps 1
plot \\'  > transport-cdf.plot.base
sed -i '$s/\\$//' transport-cdf.plot.base

cp transport-cdf.plot.base transport-cdf.plot
cat delays_all.plot >> transport-cdf.plot
sed -i '$s/, \\$//' transport-cdf.plot
touch transport-cdf.eps
gnuplot transport-cdf.plot
mv transport-cdf.eps ${BASE_NAME}-transport-cdf-${SECURITY}-${SEED}_all.eps

done
done
