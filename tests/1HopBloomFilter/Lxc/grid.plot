set term postscript
set output "overhead.ps"
set style data histograms
set style fill solid 1.0 border -1
set yrange [0:*]
set title "TX/RX Control Overhead (250 DO, 5x5 grid, 1 pub/sub pair)"
set ylabel "Bandwidth (bytes)"
plot "overhead.csv" using 2:xticlabels(1) notitle
