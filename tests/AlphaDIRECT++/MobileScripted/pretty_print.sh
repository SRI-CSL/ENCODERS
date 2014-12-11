#!/bin/bash

R=4
i=0
for f in $(ls $1*.sh_logs/apps_output); do 
    if [ ${i} -eq 0 ]; then
        echo ""
        ./get_stats.sh $f 1 
    else 
        ./get_stats.sh $f
    fi
    i=$(((i+1) % R)) 
done
