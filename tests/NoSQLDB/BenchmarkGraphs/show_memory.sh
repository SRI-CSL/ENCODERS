#!/bin/bash

logfile1=$1
logfile2=$2

timeout1=$(head -n2 "$logfile1" | tail -n1)
timeout1=${timeout1/\# /}

timeout2=$(head -n2 "$logfile2" | tail -n1)
timeout2=${timeout2/\# /}


if [[ "$2" == "smooth" ]]; then
  lineType="smooth bezier"
else
  lineType="w lines"
fi

read -r -d '' plot <<GNUPLOT
set term postscript;
set output 'memory.ps';
set title 'Memory Usage over Time';
set xlabel 'time in s';
set ylabel 'memory consumption in MB';
set key bottom right;
set grid;
plot \
'${logfile1}' using (\$0 * ${timeout1}):(\$1/1024) ${lineType} title 'sql-RSS' lc rgb "green", \
'${logfile1}' using (\$0 * ${timeout1}):(\$2/1024) ${lineType} title 'sql-Dirty' lc rgb "green", \
'${logfile1}' using (\$0 * ${timeout1}):(\$3/1024) ${lineType} title 'sql-Heap' lc rgb "green", \
'${logfile1}' using (\$0 * ${timeout1}):(\$4/1024) ${lineType} title 'sql-Stack' lc rgb "green", \
'${logfile2}' using (\$0 * ${timeout2}):(\$1/1024) ${lineType} title 'nosql-RSS' lc rgb "red", \
'${logfile2}' using (\$0 * ${timeout2}):(\$2/1024) ${lineType} title 'nosql-Dirty' lc rgb "red", \
'${logfile2}' using (\$0 * ${timeout2}):(\$3/1024) ${lineType} title 'nosql-Heap' lc rgb "red", \
'${logfile2}' using (\$0 * ${timeout2}):(\$4/1024) ${lineType} title 'nosql-Stack' lc rgb "red" ;
;
GNUPLOT

gnuplot -e "$plot"
