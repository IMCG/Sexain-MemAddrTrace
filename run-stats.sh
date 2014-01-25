#!/bin/bash

if [ $# -lt 5 ]; then
  echo "Usage: $0 FILE_PREFIX "\
    "[-e EPOCH_INTERVAL]... [-p PAGE_BITS]..."
  exit 1 
fi

file_pre=$1

files=`ls "$file_pre"*.trace`

for file in $files; do
  ./MemAddrStats.o $file ${@:2} 2>>err.log &
done
