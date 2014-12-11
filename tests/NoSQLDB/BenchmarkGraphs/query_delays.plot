set term postscript
set output "query_delays.ps"
set title "Data Objects for Node Query Delay vs Number of Nodes"
set xlabel "Node Number"
set ylabel "Delay (us)"
set grid
plot "nosql-query_delays.dat" title "no-sql" linecolor rgb "red", "sql-query_delays.dat" title "sql-mem" linecolor rgb "green"
