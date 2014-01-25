#!/bin/bash

PIN=~/pin
BUF_LEN=8388608
SKIP=100000
MAX=1000000

if [ $# -ne 1 ]; then
  echo "Usage: $0 DIR"
  exit 1
fi

DIR=$1

sudo sh -c "echo 0 > /proc/sys/kernel/yama/ptrace_scope"
mkdir -p traces

for PINBALL in `ls $DIR`
do
  nohup $PIN/pin -xyzzy -reserve_memory $DIR/$PINBALL/pinball.address \
    -t obj-intel64/MemAddrTrace.so \
    -replay -replay:basename $DIR/$PINBALL/pinball \
    -file_prefix traces/$PINBALL \
    -buffer_length $BUF_LEN -ins_skip $SKIP -ins_max $MAX \
    -- $PIN/extras/pinplay/bin/intel64/nullapp > /dev/null 2>>err.log &
done

