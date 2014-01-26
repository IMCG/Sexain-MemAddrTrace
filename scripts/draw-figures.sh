#!/bin/bash

if [ $# -ne 1 ]; then
  echo "USAGE: $0 FILE_PREFIX"
  exit 1
fi

file_pre=$1

for fullname in `ls $file_pre*.stats`
do

file=${fullname%.*}
interval=${file#*.}
interval=${interval#*-}
interval=${interval%-*}

line=`grep num_epochs $file.stats`
num_epochs=${line##*=}

gnuplot << EOF
set terminal postscript eps color font 24
set output "$file.eps"
set key below

set title '$interval Dirty Blocks/Epoch'

set style histogram clustered gap 0
set style data histograms
set style fill solid border -1

set yrange [0:1]
set ytics nomirror
set y2tics rotate by -45
set xlabel 'Epoch DR'
set xrange [-0.25:15.75]
set x2range [0:1]
set xtics nomirror rotate by -45

plot "$file.stats" u 3:xticlabels(sprintf("\%.2f", \$1)) t 'Overall DR', \
  '' u (\$4*$num_epochs):xticlabels(sprintf("\%.2f", \$1)) axes x1y2 t 'Epoch Span', \
  '' u 1:2 axes x2y1 w lines lt -1 lw 1 t ''
EOF

done
