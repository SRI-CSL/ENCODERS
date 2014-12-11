set term post eps color
set output "show-mem-usage.eps"

# display the CDF
set autoscale y
set autoscale x
set key right bottom
set title  'Memory Usage of 5k DOs'
set ylabel 'Number of Bytes Used'
set xlabel 'Delay (s)'
plot "plot-nocompact.dat" using 1:2 title 'NOCOMPACT', \
     "plot-compact.dat" using 1:2 title 'COMPACT', \
     "plot-sql.dat" using 1:2 title 'SQL'
