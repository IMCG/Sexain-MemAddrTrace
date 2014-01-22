#!/bin/bash

if [ $# -ne 1 ]; then
  echo "USAGE: $0 FILE_PREFIX"
  exit 1
fi

file_pre=$1

gnuplot << EOF
set terminal postscript eps color
set output "$file_pre.eps"

set style histogram gap 0
set style data histograms

set ylabel 'Overall Dirty Ratio'
set yrange [0:1]
set xlabel 'Epoch Dirty Ratio'
set xrange [0:16]

plot "$file_pre.stats" u 3:xticlabels(1) notitle

EOF
