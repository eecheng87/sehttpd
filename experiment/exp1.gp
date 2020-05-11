reset
set ylabel 'request/sec'
set title 'single thread vs thread pool server'
set key right top
set term png enhanced font 'Verdana,10'
set output 'exp1.png'
plot [:][:] \
'single.out' using 1:2 with points title "single thread",\
'tp.out' using 1:2 with points title "thread pool",\