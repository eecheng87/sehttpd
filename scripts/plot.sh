#!/usr/bin/env bash
# usage: repeat iterate_time cmd
DEST=$1
repeat(){
    T=$1
    shift
    for i in `seq 50 $T`;
    do
        $1 $i $2 >> $DEST
    done
}
repeat 60 "./htstress -n 10000 -c " " -t 3 http://localhost:8081/"
