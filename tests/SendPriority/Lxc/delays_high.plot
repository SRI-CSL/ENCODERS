set term postscript
set output "delays_high.ps"

# display the CDF
set autoscale y
set autoscale x
set key right top
set title  'High priority data objects delivered within delay'
set ylabel 'Average number of data objects within delay'
set xlabel 'Delay (ms)'
set yrange [0:*]

plot "enabled.HIGH.delays.dat" using 1:2 title 'enabled_HIGH', \
     "disabled.HIGH.delays.dat" using 1:2 title 'disabled_HIGH'
