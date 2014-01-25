#!/bin/bash

if [ $# -ne 1 ]; then
  echo "USAGE: $0 FILE_PREFIX"
  exit 1
fi

file_pre=$1

for fullname in `ls $file_pre*.stats`
do

file=${fullname%.*}

gnuplot << EOF
set terminal postscript eps color font 24
set output "$file.eps"
set key below box

set style histogram clustered gap 0
set style data histograms
set style fill solid border -1

set ylabel 'Ratio'
set yrange [0:1]
set xlabel 'Epoch Dirty Ratio (EDR)'
set xrange [-0.25:15.75]
set x2range [0:1]
set xtics nomirror rotate by -45

plot "$file.stats" u 3:xticlabels(sprintf("\%.3f", \$1)) t 'ODR', \
  '' u 4:xticlabels(sprintf("\%.3f", \$1)) t 'ESR', \
  '' u 1:2 axes x2y1 w lines lt -1 lw 3 t 'CDF of EDR'
EOF

done
