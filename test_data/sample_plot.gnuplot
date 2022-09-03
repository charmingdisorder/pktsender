set output "approx4_udp.png"
set term png
plot 'approx4.data' using 1:2 with lines, \
     'approx4_udp.data' using 1:2 with lines
