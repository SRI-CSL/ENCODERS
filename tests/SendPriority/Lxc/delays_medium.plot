set term postscript
set output "delays_medium.ps"

# display the CDF
set autoscale y
set autoscale x
set key right top
set title  'Medium priority data objects delivered within delay'
set ylabel 'Average number of data objects within delay'
set xlabel 'Delay (ms)'
set yrange [0:*]

plot "enabled.MEDIUM.delays.dat" using 1:2 title 'enabled_MEDIUM', \
     "disabled.MEDIUM.delays.dat" using 1:2 title 'disabled_MEDIUM'
