set output "approx7_udp.png"
set term png
plot 'approx7.data' using 1:2 with lines, \
     'approx7_udp.data' using 1:2 with lines
