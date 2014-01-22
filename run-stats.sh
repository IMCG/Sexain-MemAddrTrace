#!/bin/bash

if [ $# -ne 5 ]; then
  echo "Usage: $0 FILE_PREFIX"\
    "MIN_EPOCH_MEGA_INS MAX_EPOCH_MEGA_INS MIN_PAGE_BITS MAX_PAGE_BITS"
  echo "Note: epoch ins num is 10-based; page bit num is 2-based."
  exit 1 
fi

file_pre=$1
min_mega=$2
max_mega=$3
min_bits=$4
max_bits=$5

files=`ls "$file_pre"*.trace`

for file in $files; do
  ./MemAddrStats.o $file $min_mega $max_mega $min_bits $max_bits 2>>err.log &
done
