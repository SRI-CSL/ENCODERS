set term postscript
set output "delays.ps"

# display the CDF
set autoscale y
set autoscale x
set key right top
set title  'Data objects delivered within delay'
set ylabel 'Average number of data objects within delay'
set xlabel 'Delay (ms)'
set yrange [0:*]

plot "enabled.LOW.delays.dat" using 1:2 title 'enabled_LOW', \
     "enabled.MEDIUM.delays.dat" using 1:2 title 'enabled_MEDIUM', \
     "enabled.HIGH.delays.dat" using 1:2 title 'enabled_HIGH', \
     "disabled.LOW.delays.dat" using 1:2 title 'disabled_LOW', \
     "disabled.MEDIUM.delays.dat" using 1:2 title 'disabled_MEDIUM', \
     "disabled.HIGH.delays.dat" using 1:2 title 'disabled_HIGH'
