set term postscript
set output "node_delete_delays.ps"
set title "Node Delete Delay vs Number of Nodes Deleted"
set xlabel "Nodes Deleted"
set ylabel "Delay (us)"
set grid
set logscale y
plot "nosql-node_delete_delays.dat" title "no-sql" linecolor rgb "red", "sql-node_delete_delays.dat" title "sql-mem" linecolor rgb "green"
