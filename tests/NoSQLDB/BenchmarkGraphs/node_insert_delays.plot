set term postscript
set output "node_insert_delays.ps"
set title "Node Insert Delay vs Number of Nodes"
set xlabel "Node Number"
set ylabel "Delay (us)"
set grid
set logscale y
plot "nosql-node_insert_delays.dat" title "no-sql" linecolor rgb "red", "sql-node_insert_delays.dat" title "sql-mem" linecolor rgb "green"
