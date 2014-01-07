#!/bin/bash

if [ $# -ne 1 ]; then
  echo "USAGE: $0 FILE_PREFIX"
  exit 1
fi

file_pre=$1

gnuplot << EOF
set terminal postscript eps color
set output "$file_pre.eps"

set ylabel 'Probability'
set yrange [0:1]
set xlabel 'Dirty Proportion'
set xrange [0:1]

plot "$file_pre-8.stat" u 1:2 smooth csplines t '256-byte pages'
EOF
