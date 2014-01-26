#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Usage: $0 TRACE_DIR EPS_DIR"
  exit 1
fi

epochs=(512 1024 2048)

trace_dir=$1
eps_dir=$2

for name in `ls $1`
do
  label=${name%_*}
  echo '\begin{figure}[H]'
  echo '\centering'
  for epoch in ${epochs[*]}
  do
    stats_file=$eps_dir/$name-$epoch-12.stats
    line=`grep num_epochs $stats_file`
    num_epochs=${line##*=}
    line=`grep epoch_interval $stats_file`
    num_ins=${line##*=}
    mega=`printf "%.2f" ${num_ins%M}`

    eps_file=$eps_dir/$name-$epoch-12.eps

    echo '\subfigure[EN='$num_epochs' IPE='$mega'M] {'
    echo '  \includegraphics[width=0.3\textwidth]{'$eps_file'}'
    echo '  \label{'$label-$epoch'}'
    echo '}'
  done
  echo '\caption{'$label'}'
  echo '\end{figure}'
  echo
done
