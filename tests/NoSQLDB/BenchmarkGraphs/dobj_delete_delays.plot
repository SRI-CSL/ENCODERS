set term postscript
set output "dobj_delete_delays.ps"
set title "Data Object Delete Delay vs Number of Data Objects Deleted"
set xlabel "Data Objects Deleted"
set ylabel "Delay (us)"
set logscale y
set grid
plot "nosql-dobj_delete_delays.dat" title "no-sql" linecolor rgb "red", "sql-dobj_delete_delays.dat" title "sql-mem" linecolor rgb "green"
