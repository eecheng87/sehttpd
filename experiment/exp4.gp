# The purpose of experiment is same as exp3
# the only thing different is tried to find
# out which policy is better for single cpu

# use `taskset`

# scheme 1: execute task on queue until queue is empty
# scheme 2: execute one task and yield current thread


reset
set ylabel 'request/sec'
set title 'perf. of different scheme consuming task'
set key right top
set term png enhanced font 'Verdana,10'
set output 'exp4.png'
plot [:][:] \
'exp41.out' using 1:2 with linespoints title "scheme 1",\
'exp42.out' using 1:2 with linespoints title "scheme 2"\