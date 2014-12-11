set term postscript
set output "dobj_insert_delays.ps"
set title "Data Object Insert Delay vs Number of Data Objects"
set xlabel "Data Object Number"
set ylabel "Delay (us)"
set logscale y
set grid
plot "nosql-dobj_insert_delays.dat" title "no-sql" linecolor rgb "red", "sql-dobj_insert_delays.dat" title "sql-mem" linecolor rgb "green"
