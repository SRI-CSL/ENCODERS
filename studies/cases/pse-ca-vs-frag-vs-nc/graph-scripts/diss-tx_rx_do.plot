set term post eps color
set output "dissemination-tx_rx_do.eps"

set key left top
set title "Performance per coding policy"
set auto x
set ylabel "Total bandwidth (MB)"
set autoscale y
set ytics nomirror
set logscale y
set y2range [0:1]
set y2tics
set y2label "Delivery Fraction"

set style histogram errorbars gap 1
set style data histogram

set style fill pattern border -1
set boxwidth 0.9
set xtic scale 0
set xlabel "Coding policy"
plot 'tx_rx_do.data' using 2:3:4:xtic(1) axes x1y1 ti col, '' u 5:6:7 axes x1y1 ti col, '' u 8:9:10 axes x1y1 ti col, '' u 11:12:13 axes x1y2 ti col, '' u 14:15:16 axes x1y2 ti col
