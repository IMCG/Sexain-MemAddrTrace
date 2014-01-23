#!/bin/bash

REDIS=~/Downloads/redis-2.8.4/src/redis-server
YCSB=~/Downloads/ycsb-0.1.4
TRACER=~/Projects/Sexain-MemAddrTrace/obj-intel64/MemAddrTrace.so
OUTDIR=~/export/trace-ycsb

RECCNT=1000000 # number of records to load before benchmark
OPTCNT=1000000 # number of operations in a benchmark

if [ ! -f $REDIS ] || [ ! -d $YCSB ] || [ ! -f $TRACER ]; then
  echo "Specify proper targets please."
  exit 1
fi

if [ ! -d $OUTDIR ]; then
  mkdir -p $OUTDIR
fi

if [ -f err.log ]; then
  cat err.log >> err.log.old
  rm err.log
fi

sudo sh -c "echo 0 > /proc/sys/kernel/yama/ptrace_scope"

for WL in a b c d e f
do
  $REDIS --save "" --port 6379 2>>err.log &
  REDIS_PID=$!
  sleep 3

  $YCSB/bin/ycsb load redis -P $YCSB/workloads/workload$WL \
      -threads 10 -p recordcount=$RECCNT -s \
      -p redis.host=localhost -p redis.port=6379 > /dev/null
  sleep 3

  pin -pid $REDIS_PID -t $TRACER -file_prefix $OUTDIR/redis-ycsb-workload$WL \
      2>>err.log
  sleep 3

  $YCSB/bin/ycsb run redis -P $YCSB/workloads/workload$WL \
      -threads 10 -p operationcount=$OPTCNT -s \
      -p redis.host=localhost -p redis.port=6379 > /dev/null
  sleep 3

  kill $REDIS_PID
  sleep 3
done

