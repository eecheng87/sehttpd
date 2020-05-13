reset
set ylabel 'request/sec'
set title 'perf. of different scheme server'
set key right top
set term png enhanced font 'Verdana,10'
set output 'exp2.png'
plot [:][:] \
'single.out' using 1:2 with points title "single thread",\
'tp.out' using 1:2 with points title "thread pool",\
'lf.out' using 1:2 with points title "lock free pool,\