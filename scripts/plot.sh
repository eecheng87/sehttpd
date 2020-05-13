#!/usr/bin/env bash
# usage: repeat iterate_time cmd
DEST=$1
repeat(){
    T=$1
    shift
    for i in `seq 30 $T`;
    do
        # test several time for each Concurrecy value
        echo -n $i >> $DEST
        $1 $i $2 >> $DEST
        echo -n $i >> $DEST
        $1 $i $2 >> $DEST
        echo -n $i >> $DEST
        $1 $i $2 >> $DEST
    done
}
repeat 50 "taskset 0x2 ./htstress -n 10000 -c " " -t 3 http://localhost:8081/"
