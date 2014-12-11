set term postscript
set output "latency.ps"
set title "Latency Distribution (800 DO, 4x4 grid, rand pub-sub pairs)"
set xlabel "Latency (s)"
set ylabel "Number data objects received"
set grid
plot "nosql.dat" using 1:2 title "NoSQL" lc rgb "green", "sql.dat" using 1:2 title "SQL" lc rgb "red"
