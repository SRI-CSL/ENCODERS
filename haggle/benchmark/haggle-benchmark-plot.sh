#!/bin/bash

NAME="haggle-benchmark"
i=1
T=""


for trace in $@; do
    PARAMS=`awk 'BEGIN{i=0;} /attr_pool/ { while (i++ < 6) { split($i,a,"="); print a[2];} }' $trace`
    echo $PARAMS
    #exit
    
    #for p in $PARAMS; do
    	T="$PARAMS"
    #done

    #PLOT=$PLOT"\"$trace\" using 1:2:(\$3*2) with errorlines ls $i,"
    #PLOT=$PLOT"\"$trace\" using 4:6 with linespoints ls $i t \"$PARAMS\","
    PLOT=$PLOT"\"$trace\" using 1:2 with linespoints ls $i,"
    i=$[i+1]
    #PLOT=$PLOT"\"$trace\" using 1:4 axes x1y2 with linespoints ls $i t \"$T\","
    #PLOT=$PLOT"\"$trace\" using 4:5 axes x1y2 with linespoints ls $i notitle,"
    i=$[i+1]
done

PLOT=`echo $PLOT | sed -e 's/,$//'`
OUT=$NAME.eps

cat > $NAME.gplt <<EOF
set output "$OUT"
#set term aqua enhanced dashed font "Helvetica" 18; 
set term postscript eps enhanced mono blacktext font "Helvetica" 24;
#set term postscript eps color enhanced font "Helvetica" 18;
#set size square 0.8,1
#set multiplot 

set style data yerrorlines;
set pointsize 2;
set style line 1 lt 1 lw 3 pt 5 lc -1
set style line 2 lt 2 lw 1 pt 2 lc -1
set style line 3 lt 1 lw 3 pt 7 lc -1
set style line 4 lt 2 lw 1 pt 4 lc -1
set style line 5 lt 1 lw 2 pt 6 lc -1
set style line 6 lt 2 lw 1 pt 6 lc -1
set style line 7 lt 1 lw 2 pt 5 lc -1
set style line 8 lt 2 lw 1 pt 5 lc -1
set style line 9 lt 1 lw 2 pt 7 lc -1
set style line 10 lt 2 lw 1 pt 7 lc -1
#set xtics $xtics
#set y2tics
#set
set ylabel "query time [s]";
#set y2label "Number of matching data objects  (dashed line)";
set xlabel "number of data objects"
set yrange [:100]
#set y2range [:*]
set xrange [10:10000]
set logscale x
set logscale y
set grid;
#set label "query time" at 20,1.3
#set arrow from 40,0.9 to 200,0.2
#set label "matching data objects" at 100,0.02
#set arrow from 400,0.015 to 1400,0.01
#set arrow from 400,0.015 to 2000,0.0021
$xtracmd
#set title "$NAME"
set key top left
set notitle
plot $PLOT 
EOF

gnuplot $NAME.gplt
epstopdf --nocompress $OUT
