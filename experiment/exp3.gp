# The purpose of experiment is trying to compare
# different scheme about how to execute task
# on queue which belong to thread's

# scheme 1: execute task on queue until queue is empty
# scheme 2: execute one task and yield current thread


reset
set ylabel 'request/sec'
set title 'perf. of different scheme consuming task'
set key right top
set term png enhanced font 'Verdana,10'
set output 'exp3.png'
plot [:][:] \
'exp31.out' using 1:2 with points title "scheme 1",\
'exp32.out' using 1:2 with points title "scheme 2"\