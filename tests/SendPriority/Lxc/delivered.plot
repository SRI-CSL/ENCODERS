set term postscript
set output "delivered.ps"
set title " Data Objects Delivered By Priority "
set boxwidth 0.9 absolute
#set style fill   solid 1.00 border lt -1
set key inside right top vertical Right noreverse noenhanced autotitles nobox
set style histogram clustered gap 5 title  offset character 0, 0, 0
set datafile missing '-'
set style data histograms
set xtics border in scale 0,0 nomirror rotate by -45  offset character 0, 0, 0
set xtics  norangelimit
set ylabel "# Delivered"
set xlabel "Data Object Priority"
set xtics ('LOW' 0, 'MEDIUM' 1, 'HIGH' 2)
set yrange [0:*]
plot 'delivered.dat' u ($0-0.2/2):2:(0.2) w boxes title "disabled", \
     'delivered.dat' u ($0-0.2/2):2:3:4 w yerror notitle, \
     'delivered.dat' u ($0+0.2/2):5:(0.2) w boxes title "enabled", \
     'delivered.dat' u ($0+0.2/2):5:6:7 w yerror notitle
