#!/bin/bash

if [ $# -ne 5 ]; then
  echo "Usage: $0 FILE_PREFIX"\
    "MIN_INTERVAL MAX_INTERVAL MIN_PAGE_BITS MAX_PAGE_BITS"
  exit 1 
fi

file_pre=$1
min_interval=$2
max_interval=$3
min_bits=$4
max_bits=$5

files=`ls "$file_pre"*.trace`

for file in $files; do
  ./MemAddrStats.o $file $min_interval $max_interval $min_bits $max_bits \
      2>>err.log &
done
